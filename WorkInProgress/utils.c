#include "utils.h"

/* Globals in other file
MINODE *root;
char pathname[128], parameter[128], *name[128], cwdname[128];
char names[128][256];
int iblock;

MINODE minode[NMINODES];
MOUNT  mounttab[NMOUNT];
PROC   proc[NPROC], *running;
OFT    oft[NOFT];

*/

extern char *rootdev;
extern MINODE minode[NMINODES];
extern char blkBuf[BLOCK_SIZE];
extern int ninodes, nblocks, ifree, bfree, InodeBeginBlock;
extern GD *gp;
void get_block(int dev, int blk, char mbuf[])
{
    // using the device descriptor seek to the block we want to read i.e (blk & 1024)
    lseek(dev, blk * BLOCK_SIZE, SEEK_SET);
    read(dev, mbuf, BLOCK_SIZE);
}

int is_ext2(SUPER *sptr){
    if (sptr->s_magic != SUPER_MAGIC) {
        /* code */
        return -1;
    }
    return 1;
}
int init()
{
    int i, j;
    PROC *p;
    printf("initializing device : %s\n", rootdev );

    // Initialize minode[NMINODES] set all refCount = 0
    for (i = 0; i < NMINODES; i++) {
        /* code */
        minode[i].refCount = 0;
    }

    // Initialize mounttab[NMOUNT] set busy = 0
    for (i = 0; i < NMOUNT; i++) {
        /* code */
        mounttab[i].busy = 0;
    }

    /* Initialize proc[NPROC] set the status to FREE
       each process has a fd[NFD] file descriptor array. Set all of it to 0 */
    for (i = 0; i < NPROC; i++) {
        proc[i].status = FREE;
        for (j = 0; j < NFD; j++) {
            proc[i].fd[j] = 0;
        }
        // Finally link every process to its neighbor.
        // The proccess are essentially linked list
        proc[i].next = &proc[i +1];
    }

    // Initialize oft[NOFT] open file Table. Set the refCount = 0
    for (i = 0; i < NOFT; i++) {
        oft[i].refCount = 0;
    }

    printf("Mounting root\n");
    mount_root(rootdev);
    printf("root mounted ok!\n");

    /* Create P0 process by assiging the address of proc[0] to Proc *p
       P0 is root process its the process that executes commands */
    printf("creating P0, P1\n");
    p = running = &proc[0];                     // P0 is set as the running proc
    p->status = BUSY;                           // Set P0 status to BUSY
    p->uid = 0;
    p->pid = p->ppid = p->gid = 0;              // set pid ppid and gid to 0

    p->parent = p->sibling = p;
    p->child = 0;
    p->cwd = root;
    p->cwd->refCount++;                        // Increase the refCount by 1

    /* Create the child process */
    p = &proc[1];                              // Essentially we assign the address of proc[1] to PROC *p
                                               // Cause I am lazy and I dont want to type proc[1].Whatever everytime
    p->next = &proc[0];                        // its neighbor is parent process
    p->status = BUSY;
    p->uid = 2;
    p->pid = 1;
    p->ppid = p->gid = 0;
    p->cwd = root;
    p->cwd->refCount++;

    nproc = 2;

}
int mount_root(char *devName)
{
    // Local variables for file system operations
    int i, ino;


    char buf[BLOCK_SIZE];           // buffer for reading block
    MOUNT *mp;
    SUPER *sp;
    MINODE *ip;

    // Open device for RW
    dev = open(devName, O_RDWR);
    if (dev < 0) {
        /* code */
        printf("FAILED TO OPEN DEVICE : %s\n", devName);
        exit(EXIT_FAILURE);
    }
    printf("device %s opened successfully dev descriptor = %d\n", devName, dev);

    // read super block into blkBuf
    get_block(dev, SUPERBLOCK, buf);
    sp = (SUPER *)buf;

    // check that it is indeed ext2 fs
    if(is_ext2(sp) < 0)
    {
        printf("ERROR: NOT EXT2 FS\n");
        exit(EXIT_SUCCESS);
    }


    // Check to see if SUPER *sp is initialized
    printf("s_magic = %x : SUPER_MAGIC = %x\n", sp->s_magic, SUPER_MAGIC );

    /*Setup mounttab[0] this will be our root device and will contain
      nodes and block information */
    mp = &mounttab[0];

    /* save important inode and block information
       copy super block info to mounttab[0] */
    ninodes = mp->ninodes = sp->s_inodes_count;
    nblocks = mp->nblocks = sp->s_blocks_count;

    // save the free inodes and free blocks
    ifree = sp->s_free_inodes_count;
    bfree = sp->s_free_blocks_count;
    /* printf("nblock count  = %d\n", nblocks);
    // printf("ninodes count = %d\n", ninodes);
    // printf("free inodes   = %d\n", ifree);
    // printf("free blocks   = %d\n", bfree);
    */

    // Get Block 2 Group descriptor Block
    get_block(dev, GDBLOCK, buf);
    gp = (GD *)buf;

    // Assign mounted device to the dev descriptor and set it to busy
    mp->dev = dev;
    mp->busy = BUSY;

    /*Copy the group descrip bmap and imap and InodeBeginBlock (Inode Table)
      into mounttab[0] */
    mp->bmap    = bmap = gp->bg_block_bitmap;
    mp->imap    = imap = gp->bg_inode_bitmap;


    mp->iblock  = InodeBeginBlock = gp->bg_inode_table;     // Save the inode begin block
    strcpy(mp->name, devName);                              // copy the root device name into mounttab[0]
    strcpy(mp->mount_name, "/");                            // copy the '/' root to mounttab[0] device name

    printf("bmap = %d", gp->bg_block_bitmap);
    printf("imap = %d", gp->bg_inode_bitmap);
    printf("InodeBeginBlock (iblock) = %d \n",gp->bg_inode_table);

    /***** call iget(), which inc the Minode's refCount ****/
    root = iget(dev, 2);        // Get root's MINODE inode #2
    mp->mounted_inode = root;   // set the mounttab[0] mounted inode to root

    // print information about the mounted device
    printf("mount: %s mounted on / \n", devName);
    printf("nblocks = %d, bfree = %d ninodes = %d ifree = %d\n", nblocks, bfree, ninodes, ifree);

    return 0;

}

/*6. MINODE *iget(int dev, int ino)
{
  Once you have the ino of an inode, you may load the inode into a slot
  in the Minode[] array. To ensure uniqueness, you must search the Minode[]
  array to see whether the needed INODE already exists:
  If you find the needed INODE already in a Minode[] slot, just inc its
  refCount by 1 and return the Minode[] pointer.
  If you do not find it in memory, you must allocate a FREE Minode[i], load
  the INODE from disk into that Minode[i].INODE, initialize the Minode[]'s
  other fields and return its address as a MINODE pointer,
}
*/
MINODE *iget(int dev, uint32_t ino)
{
    // locals
    MINODE *tmip;                 // Temp mip pointer to minode []
    int blk, offset;
    char mbuf[BLOCK_SIZE];


    int i=0;
    while (i < NMINODES) {        // NMINODES global constant = 50 can change in type.h file
        // if this inode exists in the Minode[] and the refCount is not 0
        if (minode[i].refCount > 0 && minode[i].ino == ino) {
            tmip = &minode[i];        // Assign the address of this minode[i] to a temp minode pointer
            minode[i].refCount++;
            return tmip;
        }
        i++;
    }

    // Inode does not exists search for a free location to place this inode
    i = 0;
    // loop till refCount = 0 or i == NMINODES (No space in minode[] array)
    while (minode[i].refCount > 0 && i < NMINODES) { i++; }
    if (i == NMINODES) {
        //printf("Error: MINODE[] IS FULL!\n");
        return 0;
    }

    // get the inode block iBlock by using mailman's algorithm
    blk = (ino - 1) / 8 + InodeBeginBlock;
    //printf("iget() blk = %d\n", blk);

    // get the offset of the inode
    offset = (ino - 1) % 8;
    //printf("iget() offset = %d\n", offset);


    // Find this blk
    get_block(dev, blk, mbuf);
    ip = (INODE *)mbuf + offset;      // + offset to get the location of this inode within the block


    // Copy the inode from disk into the minode[i] : free location that we found earlier
    memcpy(&(minode[i].INODE), ip, sizeof(INODE));

    // Set the information for minode[i] array
    minode[i].dev = dev;
    minode[i].ino = ino;
    minode[i].refCount = 1;
    minode[i].dirty = 0;
    minode[i].mounted = 0;
    minode[i].mountptr = 0;

    // return the address of this minode[i]

    //printf("iget() Working!\n");

    return &minode[i];
}
int iput(int mdev, MINODE *mip)
{
    char buf[BLOCK_SIZE];
    int blk, offset;
    INODE *ip;

   // printf("iput(): dev = %d\n", mdev);
    //printf("iput() decrementing inode refCount = %d\n", mip->refCount);

    // Step 1: decrease the inode refCount by 1
    mip->refCount--;
    if(mip->refCount > 0)
    {
        //printf("iput() refCount = %d\n", mip->refCount);
        return 0;
    }

    // Step 2: check if the Inode has not been modified
    if(mip->dirty == 0)
    {
        //printf("iput() inode has not been modified : dirty = %d\n", mip->dirty);
        return 0;           // No need to put Inode back into device since it has not been modified
    }

    // Step 3: Write Inode back into disk using MailMan's Algorith to find blk and offset
    blk     = (mip->ino - 1) / InodeBeginBlock;
    offset  = (mip->ino -1) % 8;
    //printf("Preparing to write inode to disk @ blk = %d offset = %d\n", blk, offset);

    // Step 4: read the block into the buffer using the blk number
    get_block(mip->dev, blk, buf);
    ip = (INODE *)buf;

    // Step 5: Copy the In_Memory INODE struct into ip (INODE inside buffer array)
    memcpy(ip, &(mip->INODE), sizeof(INODE));

    // Step 6: Write the block back into disk
    put_block(mip->dev, blk, buf);

    return 0;

}

int put_block(int mdev, int blk, char *buf)
{
    lseek(mdev, (long)(blk * BLOCK_SIZE), SEEK_SET);
    write(mdev, buf, BLOCK_SIZE);

    return 0;
}


/*
This is the most important function of the FS. It converts a pathname, such as
 /a/b/c/d OR x/y/z, into its inode number (ino).

 THIS IS THE SAME AS YOUR showblock.c program. Instead of printing blocks,
 return its inode number.
*/
uint32_t getino(int dev, char *path)
{
    uint32_t ino = 0;
    char **names = {0};               // Holds the tokens of path i.e. /a/b/c/ -> [a][b][c]...
    MINODE *mip = 0;
    //printf("******************************\n");
    printf("getino() path = %s\n", path);

    if (path[0]) {

        names = tokenize(path);     // Break path up into tokens
    }
    else{       // path is not given

        /* ino is the running proccess current working directory.
        it is where we are currently in the dirs i.e. if we cd cs360 then
        the ino belongs to cs360 directory
        */
        ino = running->cwd->ino;
        return ino;
    }



    // No if the path begins with '/' character then we are starting from root
    // Absolute directories always has path starting with '/'
    if (path[0] == '/') {
        ip = &root->INODE;        // inode pointer points to the root INODE
        ino = root->ino;            // inode number is root->ino
    }
    else{
        // Start from cwd: Relative paths do not start with '/' i.e. cs360/is/fun
        // We need to find the inode number for the last dir in the path i.e. fun
        ip = &running->cwd->INODE;
    }



    int i= 0;
    // search for ino of each path in the names[][]
    while (names[i] != 0) {
        //printf("searching ino of %s\n", names[i]);
        ino = search(dev, names[i], ip);            // remember: ip = &running->cwd->INODE; where running is a PROC *running

        if(ino <= 0){
            // The path doesnt exits put mip back into the device
            if(mip)
            {
                iput(mip->dev, mip);
                return -1;
            }
        }
       // printf("[%s] ino = %d\n", names[i], ino);

        if(mip){
            //printf("put mip back into device\n");
            iput(mip->dev, mip);
        }

        //printf("get the mip for: name = %s device = %d ino = %d\n", names[i], dev, ino);

        i++;
        if(names[i] != 0)
        {
            //printf("names[i] exists = %s\n", names[i]);
            mip = iget(dev, ino);
            ip = &mip->INODE;
        }
        //printf("getino() next i = %d\n", i);

    }

    //printf("No other names[] i = %d\n", i);

    i = 0;
    while (names[i]) { free(names[i++]); }

    printf("return: ino = %d\n", ino);
    return ino;
}
uint32_t search(int mdev, char *name, INODE *ip)
{
    int i = 0;
    char *cp;
    DIR *dp;
    char mbuf[BLOCK_SIZE], tempEnt[256];

    // search the first 12 blocks DIRECT BLOCKS
    while (i < 12) {
        if(ip->i_block[i] == 0){ break; }  // i_block[i] is empty

        // read the block into buffer
        get_block(dev, ip->i_block[i++], mbuf);
        dp = (DIR *)mbuf;
        cp = mbuf;

        while (cp < mbuf + BLOCK_SIZE) {
            bzero(tempEnt, 256);
            strncpy(tempEnt, dp->name, dp->name_len);

            // Match the name of the directory with the dp->name
            printf("compare: name = %s tempEnt = %s\n", name, tempEnt);
            if(strcmp(tempEnt, name) == 0) {
                printf("search() return: %d\n", dp->inode);

                // Inode belongs to the name we are looking for
                return dp->inode;
            }

            // Next entry
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }

    // if we reach here then the name was not found
    return 0;
}

/* Gets the local inode's string name using the the parent inode pointer
  to look through the parents inode direct blocks until found then
  return the name through perameter pointer. Return 1 if found else 0
 */
uint32_t getinoname(MINODE *parent, int myinode, char *inoname)
{
	int i = 0;
	int found = 0;
	char tempname[128];
	char buf[BLOCK_SIZE];
	char *cp;
	DIR *localdirinoptr;
	INODE *localinoptr;
	//
	localinoptr = &parent->INODE;

	for(i = 0; found == 0 && i < 12; i++)
	{
		get_block(dev, localinoptr->i_block[i], buf);
		cp = buf;
		localdirinoptr = (DIR *)buf;
		strcpy(tempname, localdirinoptr->name);

		// ensure tempname ends with null terminator
		tempname[localdirinoptr->name_len] = '\0';

		// test
		printf("Inside get inode name function\n");


		while(cp < (buf + BLOCK_SIZE))
		{
			// Right Dir entery found!
			if(localdirinoptr->inode == myinode)
			{
				strcpy(inoname, tempname);
				found = 1;
				break;
			}

			// Increment cp to next address in buff
			cp += localdirinoptr->rec_len;
			localdirinoptr = (DIR *)cp;
			strcpy(tempname, localdirinoptr->name);
			// ensure tempname ends with null terminator
			tempname[localdirinoptr->name_len] = '\0';

		}

	}

	return found;
}
/* mip (memory inode ptr) is the inode data member (minode) we are looking into
 * and myinode is the current inode number's save location and
 * parentino is parent inode numbers's save location.
 * This function takes in mip and gets it's and it's parents inode
 * number.
*/
uint32_t findinode(MINODE *mip, int *myinode, int *parentino)
{
	char *cp;
	char buf[BLOCK_SIZE];
	DIR *localdirinoptr;
	INODE *localinoptr;

	// test
	printf("Inside find inode function\n");

	localinoptr = &mip->INODE;
	get_block(mip->dev, localinoptr->i_block[0], buf);
	cp = buf;

	// set myinode pointer
	localdirinoptr = (DIR *)buf;
	*myinode = localdirinoptr->inode;

	// set myinodes parents pointer
	cp += localdirinoptr->rec_len; // Increment cp to next address in buff
	localdirinoptr = (DIR *)cp;
	*parentino = localdirinoptr->inode;

	return 0;
}
/*
  tokenize a pathname into components and their numbers n.
  Store the components in names[64][64] and let name[i] point at names[i];
  The components will be used to search for the inode of a pathname.
*/
char ** tokenize(char *path)
{

    int i = 0;
    char *token, *pathCopy;            // Holds a path name

    // A 2Dimensional Array char *names[256]
    char **pathArr = NULL;
    pathArr = (char **)malloc(sizeof(char *) * 256);

    // preserve the pathname
    pathCopy = strdup(path);

    char *temp;
    token = strtok(pathCopy, "/");
    while (token) {
        temp = (char*)malloc(sizeof(char) * strlen(token));
        strcpy(temp, token);
        pathArr[i++] = temp;
        token = strtok(0, "/");
    }

    pathArr[i] = 0;

    return pathArr;
}
void getInput()
{
    char buf[1024];
    int len = -1;
    char *temp;

    printf("cs360@mysh: ");
    fgets(line, 1024, stdin);
    line[strlen(line) - 1] = 0;     // kill \n
}

/***************** Bit Functions **************/
/* The i in the perameters is the bit, buf is the buffer we are searching into.
 * We use mailmans algorithm. Note 1 bit reps a block of size 4kB*/
int tst_bit(char *buf, int i)
{

    printf("tst_bit()\n");
    int byte, offset;
    // Step 1: Use mailman's  algorithm
    byte = i / 8;
    offset = i % 8;

    // bit is set to 0 or 1
    int bit = (*(buf + byte) >> offset) & 1;

    //printf("tst_bit(): bit = %d\n", bit);

    return bit;
}
int set_bit(char *buf, int i)
{
    int byte, offset;
	byte = i/8;
	offset = i%8;
	buf[byte] |= (1 << offset);
	return 0;
}
// Sets bit to 0 to say inode/block is free
int unset_bit(char *buf, int i)
{
	int byte, offset;
	byte = i/8;
	offset = i%8;
	buf[byte] &= (0 << offset);
	return 0;
}

/***************** Inode,Block Alloc/Dealloc Functions **************/

// Mo: This function decriments the free_inodes_count members of
// super and group discriptor blocks by 1
void dec_free_inodes(int dev)
{

	char buf[BLOCK_SIZE];


	printf("Decrimented free inodes count from SUPER and GD by 1\n");
	// Decriment from Super structure.
	get_super_block(dev, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count--;
	put_block(dev, SUPERBLOCK, buf);

	// Decriment from Group Discriptor structure.
	get_gd_block(dev, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	put_block(dev, GDBLOCK, buf);
}
// Mo: This function incriments the free_inodes_count members of
// super and group discriptor blocks by 1
void inc_free_inodes(int dev)
{
	char buf[BLOCK_SIZE];


	printf("Incrimented free inodes count from SUPER and GD by 1\n");

    get_super_block(dev, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count++;
	put_block(dev, SUPERBLOCK, buf);

	// Incriment from Group Discriptor structure.
	get_gd_block(dev, buf);
	gp = (GD *)buf;

	gp->bg_free_inodes_count++;
	put_block(dev, GDBLOCK, buf);
}
// Mo: This function decriments the free_blocks_count members of
// super and group discriptor blocks by 1
void dec_free_blocks(int dev)
{
	SUPER *sp;
	GD *gp;
	char buf[BLOCK_SIZE];

	// Decriment from Super structure.
	get_block(dev, SUPERBLOCK, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count--;
	put_block(dev, SUPERBLOCK, buf);

	// Decriment from Group Discriptor structure.
	get_block(dev, GDBLOCK, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count--;
	put_block(dev, GDBLOCK, buf);

	printf("Decrimented free blocks count from SUPER and GD by 1\n");
}
// Mo: This function incriments the free_blocks_count members of
// super and group discriptor blocks by 1
void inc_free_blocks(int dev)
{
	char buf[BLOCK_SIZE];

	// Incriment from Super structure.
	get_super_block(dev, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count++;
	put_block(dev, SUPERBLOCK, buf);

	// Incriment from Group Discriptor structure.
	get_gd_block(dev, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count++;
	put_block(dev, GDBLOCK, buf);

	printf("Incrimented free blocks count from SUPER and GD by 1\n");
}
//Mo: Utility function to allocate an Inode
// & returns inode number if the're free inodes
int ialloc(int pdev)
{
    printf("Allocating inode in dev %d\n", pdev);
    char buf[BLOCK_SIZE];

    printf("reading imap block #%d from disk\n", imap);
    // Step 1: get the imap block from disk into the buf
    get_block(pdev, imap, buf);
    printf("ninodes = %d\n", ninodes);

    // ninodes is global number of inodes in disk.
    int i = 0;
    while(i < ninodes)
    {
        // Find a free inode bitmap
        if(tst_bit(buf, i) == 0)
        {
            printf("free bit found: i = %d\n", i);

            // Step 2: set the bit
            set_bit(buf, i);
           // Step 3: Write inode changes back into fs structure
			put_block(pdev, imap, buf);

			 // Step 4: decriment # of free inodes since we're alloc
			dec_free_inodes(pdev);

			// Step 5:Return inode number location
			return i+1;
        }

        i++;
    }
	printf("no more free inodes in ialloc()\n");
	return 0;
}
//Mo: Utility function to allocate a block number
// given the file discriptor pdev & returns block number
int balloc(int pdev)
{
	char buf[BLOCK_SIZE];
	printf("Allocating block in dev %d\n", pdev);
	printf("reading block bit map #%d from disk\n", bmap);
    // Step 1: get the bmap block from disk into the buf
    get_block(pdev, bmap, buf);

	// nblocks is global number of blocks in disk.
    int i = 0;
    while(i < nblocks)
    {

        // Find a free block from bitmap
        if(tst_bit(buf, i) == 0)
        {
            printf("free bit found: i = %d\n", i);

            // Step 2: set the bit
            set_bit(buf, i);
            // Step 3: decriment # of free blocks since we're alloc
		    dec_free_blocks(pdev);

		    // Write 0's into buffer
		    bzero(buf, BLOCK_SIZE);

			// Step 4: Write block changes back into fs structure
			put_block(pdev, bmap, buf);

			// Step 5:Return block number location
			return i+1;
        }

        i++;
    }
	printf("no more free blocks in balloc()\n");

    return 0;
}
// deallocates inode given a inode number, ino
int idealloc(int dev, int ino)
{
	char buf[BLOCK_SIZE];

	// Read inode bitmap to buffer
	get_block(dev, imap, buf);
	// set bit in ino location in buffer to 0 since dealloc
	unset_bit(buf, ino);

	// Write inode changes back into fs structure
	put_block(dev, imap, buf);

    // decriment # of free inodes cause dealloc
	inc_free_inodes(dev);
	return 0;
}
// deallocates block given a block number, bno
int bdealloc(int dev, int bno)
{
	char buf[BLOCK_SIZE];

	// Read block bitmap to buffer
	get_block(dev, bmap, buf);
	// set bit in bno location in buffer to 0 since dealloc
	unset_bit(buf, bno);
	// Write block changes back into fs structure
	put_block(dev, bmap, buf);

	// decriment # of free blocks cause dealloc
	inc_free_blocks(dev);
	return 0;
}

int get_super_block(int dev, char *buf)
{
    get_block(dev, SUPERBLOCK, buf);
}
int get_gd_block(int dev, char *buf)
{
    get_block(dev, GDBLOCK, buf);
}


/***************** Commands Functions **************/

/*5. ls [pathname] command:
{
      int ino, dev = running->cwd->dev;
      MINODE *mip = running->cwd;
      if (pathname){   // ls pathname:
          if (pathname[0]=='/')
             dev = root->dev;
          ino         = getino(&dev, pathname);
          MINODE *mip = iget(dev, ino);
      }
      // mip points at minode;
      // Each data block of mip->INODE contains DIR entries
      // print the name strings of the DIR entries
   }
*/

int ls(char *path)
{
/* local variables */
    uint32_t ino;                // the inode number of path
    MINODE *mip;            // pointer to the inode inside minode[] array
    char *dirc, *basec, *parent, *child;

    //printf("cmd = ls path = %s \n", pathname);
    getchar();

    /* Case 1: pathname is null set inode number to the running->cwd->ino
    // The Inode we are interested in is the current running process inode number
       Case 2: pathname begins with / and  pathname[1] is black
       Case 3: pathname is other than just / i.e. /a/b/c a/b/c then we have to find the ino
    */
    if(!path || !pathname[0])
    {
        ino = running->cwd->ino;
        //printf("running->cwd->ino = %d\n", ino);
    }
    else if(pathname[0] == '/' && pathname[1] == 0)
    {
        ino = root->ino;
    }
    else
        ino = getino(dev, path);

    if(!ino)
    {
        //printf("Invalid pathname\n");
        return -1;
    }

    // Find the dirname and basenaem
    dirc    = strdup(path);
    basec   = strdup(path);
    parent  = dirname(dirc);
    child   = basename(basec);



    // // Get the indoe from minode[]
    mip = iget(dev, ino);
    printf("ls() dev = %d ino = %d\n", mip->dev, ino);

    // // Each data block of mip->INODE contains DIR entries
    if(S_ISDIR(mip->INODE.i_mode))
    {
       ls_dir(mip, child);
    }
    else
    {
        ls_file(mip, child);
    }

    iput(mip->dev, mip);
    getchar();

}
int ls_dir(MINODE *mip)
{
    INODE *ip = &mip->INODE;         // Saves typing
    MINODE *cip;                     // child inode pointer

    int mdev = mip->dev;

    DIR *dp;
    char buf[BLOCK_SIZE], *cp;

    int i_blks = ip->i_size / BLOCK_SIZE;
    printf("ls directory: dev = %d blksize = %d\n", dev, i_blks);

    int i = 0;

    // Search i_blocks of INODE 0 - 11.. We dont need indirect and double indirect blocks
    while(i < 12)
    {
        if(ip->i_block[i] == 0 || i >= 12)
            break;

        get_block(mdev, ip->i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;

        while(cp < buf + BLOCK_SIZE)
        {
            char *temp = strdup(dp->name);
            temp[dp->name_len] = 0;

            cip = iget(mdev, dp->inode);
            ls_file(cip, temp);

            //printf("ls done! \n");

            iput(mdev, cip);

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        i++;
    }

    return 0;

}

int ls_file(MINODE *mip, char *file)
{

    static const char *permissions = "rwxrwxrwx";
    char *str_time;

    if(mip == NULL)
    {
        printf("Error: Memory Inode is Null\n");
        return -1;
    }

    INODE *ip = &mip->INODE;

    // Get info from INODE Struct
    uint16_t mode   = ip->i_mode;
    uint16_t links  = ip->i_links_count;
    uint16_t uid_t  = ip->i_uid;
    uint16_t gid    = ip->i_gid;
    uint16_t size   = ip->i_size;

    str_time = ctime((time_t *)&ip->i_mtime);
    str_time[strlen(time) - 1] = 0;

    switch(mode & 0xF000)
    {
        case 0x8000: putchar('-');      // regular file
            break;
        case 0x4000: putchar('d');      // directory
            break;
        case 0xA000: putchar('l');      // link
            break;
        default:    putchar('$');
    }


    int i = 0, len = strlen(permissions);
    while(i < len)
    {
        if(mode & (1 << len - 1 - i))
        {
            putchar(permissions[i]);
        }
        else
        {
            putchar('-');
        }

        i++;
    }

    // Display file inode information
    printf("%4u %4u %4u %4u %4u %s\n", mode, links, uid_t, gid, size, file);

    return 1;


}




int cd(char *path)
{
    MINODE *mip;
    uint32_t ino;

    printf("cd() : path = %s\n", path);
    if(!path || !path[0] || (path[0] == '/' && !path[1]))
    {
        ino = root->ino;
        printf("ino = root->ino = %d\n", ino);
    }
    else{
        ino = getino(dev, path);
        printf("ino = getino() : ino = %d\n", ino);
    }

    if (ino == 0) {
        printf("Invalid Path\n");
        return -1;
    }

    // // get the inode from minode[] array
    mip = iget(dev, ino);
    printf("cd(): mip->dev = %d\n", mip->dev);
    printf("checking if %s is a directory!\n", path);
    getchar();

    // // We can only cd into a directory. Verify that the path provided is indeed a directory
    if(!S_ISDIR(mip->INODE.i_mode))
    {
        printf("Error: given path is not a directory\n");
        iput(dev, mip);             // put the inode back into the disk
        return -1;
    }

    printf("%s is a directory\n", path);

    // if it is a directory then put the inode back into device
    // Change current working directory to the mip inode in minode[]
    // that belongs to the path give.
    iput(dev, running->cwd);
    running->cwd = mip;
}


/** HOW TO MKDIR **
Assume: command line = "mkdir pathname" OR  "creat pathname"
Extract cmd, pathname from line and save them as globals.
    make_dir()
    {
    1. pahtname = "/a/b/c" start mip = root;         dev = root->dev;
                =  "a/b/c" start mip = running->cwd; dev = running->cwd->dev;
    2. Let
         parent = dirname(pathname);   parent= "/a/b" OR "a/b"
         child  = basename(pathname);  child = "c"
    3. Get the In_MEMORY minode of parent:
             pino  = getino(&dev, parent);
             pip   = iget(dev, pino);
       Verify : (1). parent INODE is a DIR (HOW?)   AND
                (2). child does NOT exists in the parent directory (HOW?);
    4. call mymkdir(pip, child);
    5. inc parent inodes's link count by 1;
       touch its atime and mark it DIRTY
    6. iput(pip);
    }
*/

int make_dir(char *path)
{
    char *dirc, *basec, *child, *parent;

    MINODE *mip;
    INODE *pip;
    int pdev, ino;

    printf("mkdir(): path = %s\n", path);

    // Step 1: check if the pathname is absolute or relative to current working directory
    if(path[0] == '/')
    {
        pdev = root->dev;           // start from root
    }
    else
    {
        pdev = running->cwd->dev;   // start from current working directory
    }


    // Step 2: get the parent (dirname ) and child (basename)
    // if directory is /a/b/c the parent = /a/b child = c or if directory a/b/c then parent is a/b and child is c
    // if command is mkdir C then parent is . and child is C
    dirc = strdup(path);
    basec = strdup(path);
    parent = dirname(dirc);
    child = basename(basec);

    printf("dirname=%s, basename=%s\n", parent, child);


    // Step 3: Get the parent ino using dev
    ino = getino(pdev, parent);
    printf("parent ino = %d", ino);

    // Make sure ino is valid
    if(ino <= 0)
    {
        printf("Error: path does not exists\n");
        return -1;
    }

    // Step 4: Get the In_MEMORY minode of parent:
    mip = iget(pdev, ino);
    pip = &mip->INODE;
    printf("in_MEMORY minode of parent: dev = %d\n", mip->dev);
    getchar();
    // Step 5: Verify parent INODE is a DIR (HOW?)

    if(!S_ISDIR(mip->INODE.i_mode))
    {
        printf("Parent Path is not a directory\n");
        // put inode back into dev
        iput(pdev, mip);
        return -1;
    }

     //Step 6: Verify  child does NOT exists in the parent directory (HOW?);
     // By calling the search function. If the ino returned by the search()
     // then the directory exists
    ino = search(pdev, child, pip);
    if(ino > 0)
    {
        printf("ERROR: Directory with name \"%s\" already exists.\n", child);
        // put inode back into dev
        iput(pdev, mip);
        return -1;
    }

    getchar();
    printf("make_dir: child = %s ino = %d\n", child, ino);

    // Step 7: Call my_mkdir() passing parent in_MEMORY minode of parent and the child (basename)
    my_mkdir(mip, child); // mip is parent minode ptr

	// inc parent inodes's link count by 1
    mip->INODE.i_links_count++;
	// touch its atime and mark it DIRTY
	mip->INODE.i_atime = time(0L);
	mip->dirty = 1;
	// put inode back into dev
    iput(pdev, mip);
    return 0;
}
//Mo: This function is similer to mkdir except it has a few different values in its
// inode fields and has no data blocks
int creat_file(char *path)
{
	int pdev,ino, PInoNum; // Parent Inode Number
	char *dirc, *basec, *child, *parent;
    MINODE *mip; // Parent MINODE ptr


    printf("creat path = %s\n", path);

    // Step 1: check if the pathname is absolute or relative to current working directory
    if(path[0] == '/')
    {
        pdev = root->dev;           // start from root
    }
    else
    {
        pdev = running->cwd->dev;   // start from current working directory
    }


    // Step 2: get the parent (dirname ) and child (basename)
    // if directory is /a/b/c the parent = /a/b child file = c or if directory a/b/c
    // then parent is a/b and child fileis c
    // if command is creat C then parent is . and child file is C
    dirc = strdup(path);
    basec = strdup(path);
    parent = dirname(dirc);
    child = basename(basec);

    printf("creat() dirname=%s, basename=%s\n", parent, child); // Test

	// Get the parents inode number & MINODE ptr
	PInoNum = getino(pdev, parent);
	mip = (MINODE *)iget(pdev, PInoNum);

	printf("in_MEMORY minode of parent: dev = %d\n", mip->dev);

    //  Verify parent INODE is a DIR (HOW?)
    if(!S_ISDIR(mip->INODE.i_mode))
    {
        printf("creat(): Parent Path is not a Directory\n");
        // put inode back into pdev
        iput(pdev, mip);
        return -1;
    }

    // Verify  child does NOT exists in the parent directory (HOW?);
    // By calling the search function. If the ino returned by the search() != 0
    // then the file exists
    ino = search(pdev, child, &mip->INODE);
    if(ino > 0)
    {
        printf("creat(): ERROR: File with name %s already exists.\n", child);
        // put inode back into dev
        iput(pdev, mip);
        return -1;
    }

    my_creat(mip, child);

    // Set inode time & PMIno dirty to 1
    mip->INODE.i_atime = time(0L);
    mip->dirty = 1;

	// Finally write minode to disk
	iput(pdev, mip);

	return 0;
}


// This function handles the dir entry into the MINODE, INODE, DIR fs structures.
// pip points at the parent minode[] of "/a/b", name is a string "c")
int my_mkdir(MINODE *pip, char *name)
{
    int i, RemainingSpace, InodeNum, BlockNum;
    MINODE *mip;
    INODE *ip;
    DIR *dp;
	char *cp;
    char buf[BLKSIZE];


    InodeNum = ialloc(dev);
    BlockNum = balloc(dev);

    printf("my_makedir: get In_MEMORY minode\n");
    // to load the inode into a minode[] (in order to
    // wirte contents to the INODE in memory).
    mip = iget(pip->dev, InodeNum);
    ip = &(mip->INODE);                     // Save on typing effors

    printf("my_makedir: set: mode, uid, gid, size, link_counts\n");
    //  Use ip-> to acess the INODE fields:
    ip->i_mode = 0040775;         // OR 040755: DIR type and permissions DIR_MODE 0040775
    ip->i_uid  = running->uid;   // Owner uid
    ip->i_gid  = running->gid;   // Group Id
    ip->i_size = BLKSIZE;        // Size in bytes
    ip->i_links_count = 2;       // Links count=2 because of . and ..

    printf("my_makedir: set: access time, creation time, mount time\n");
     // set to current time
    ip->i_atime = time(0L);
    ip->i_ctime = time(0L);
    ip->i_mtime = time(0L);

    printf("my_makedir: set reserved blocks for ip->i_blocks, i_block[0] = BlockNum\n");
    ip->i_blocks = BLKSIZE / 512;               // LINUX: Blocks count in 512-byte chunks  reserve this many blocks for inode
    ip->i_block[0] = BlockNum;                  // new DIR has one data block

    // Clear the Data blocks: i_blocks 1-14
    i = 1;
    while(i < 15)
    {
        ip->i_block[i] = 0;
        i++;
    }

    // mark minode dirty
    mip->dirty = 1;
    printf("my_makedir: write changes to disk\n");
    // write the new INODE to disk.
    iput(dev, mip);

    // Write . and .. entries into a buf[ ] of BLKSIZE
    get_block(dev, BlockNum, buf);
    cp = buf;
    dp = (DIR *)buf;

    printf("my_makedir: write \".\" entry to disk\n");
    // For . entry
    dp->inode = InodeNum;
    dp->rec_len = ideal_record_length(strlen("."));
    dp->name_len = strlen(".");
    dp->file_type = EXT2_FT_DIR;            // EXT2_FT_DIR = 2 declared in ext2_fs.h

    // Finally Write the directory entry name for .
    strcpy(dp->name, ".");

    printf("my_makedir: write '..' entry to disk\n");
    // For .. entry
    cp += dp->rec_len;
    dp = (DIR *)cp;
    dp->inode = InodeNum;
    dp->rec_len = ideal_record_length(strlen(".."));
    dp->name_len = strlen("..");
    dp->file_type = EXT2_FT_DIR;            // EXT2_FT_DIR = 2 declared in ext2_fs.h

    // Finally Write the directory entry name for .. entry
    strcpy(dp->name, "..");

    printf("my_makedir: put_block into the disk\n");
    // Then, write buf[ ] to the disk block BlockNum
    put_block(dev, BlockNum, buf);


    printf("my_makedir: put name into disk\n");
    // Finally, enter name ENTRY into parent's directory
    // What we did before was for the default . and .. entries WITHIN the newly created
    // directory. Now, we are adding in the new entry itself into the PARENT's i_block[] array
    enter_name(pip, InodeNum, name);

    return 1;
}

int ideal_record_length(const int name_len)
{
    return (4 * ((8 + name_len + 3) / 4));
}

int needed_length(const int name_len)
{
    // the needed length is
   return (4 * ((8 + name_len + 3) / 4));
}
// This subsidary function creates a file by adjusting the
// LocalMinodePtr->INODE and MINODE fields then writing the edited minode
// back onto the disk.
int my_creat(MINODE *pip, char *name)
{
	int i = 1, InodeNum;
    INODE *LocalInodePtr;
    MINODE *LocalMinodePtr;

    InodeNum = ialloc(dev);

	// to load the inode into a minode[] (in order to
    // wirte contents to the INODE in memory).
    LocalMinodePtr = (MINODE *)iget(dev, InodeNum);

    // Read mip-INODE to access/modify it fields
    LocalInodePtr = &LocalMinodePtr->INODE;

    //  Use ip-> to acess the INODE fields:
    LocalInodePtr->i_mode = 0100644;         // OR 0100644: REG type and permissions
    LocalInodePtr->i_uid  = running->uid;   // Owner uid
    LocalInodePtr->i_gid  = running->gid;   // Group Id
    LocalInodePtr->i_size = 0;        		// Size in bytes, NO data blocks so size = 0
    LocalInodePtr->i_links_count = 1;       // Links count=1 because it's a file
    // set to current time
    LocalInodePtr->i_atime = LocalInodePtr->i_ctime = LocalInodePtr->i_mtime = time(0L);
    LocalInodePtr->i_blocks = 2;            // LINUX: Blocks count in 512-byte chunks
	LocalInodePtr->i_block[0] = 0;          // new file doesn't have any data blocks

    // Clear the iblocks
    while(i < 15)
    {
        LocalInodePtr->i_block[i] = 0;
        i++;
    }
	// mark minode dirty
    LocalMinodePtr->dirty = 1;
    // write the new INODE out to disk.
    iput(dev, LocalMinodePtr);

	// Last put file name entry into the parent(pip) minode -> inode's i_blocks[]
	enter_name(pip, InodeNum, name);
	return 0;
}


/*Mo: This function adds a name (folder name..) entry to the parent
 * minode pointer's next availible data block. If the current data block
 * with the given name is full then we create a disk block at the next
 * index of i_block.
 * Note: does only direct blocks
 p_mip = parents memory inode pointer

 */
int enter_name(MINODE *parentMinoPtr, int ino, char *name)
{
    int i = 0, freeSpace, tempBlockNum, reqLen, idx;
	char *prev_cp, *cp;
	char buf[BLOCK_SIZE];
	DIR *newDirPtr, *locDirPtr;
	INODE *locInoPtr;

	locInoPtr = &parentMinoPtr->INODE;

	while(i < 12 && locInoPtr->i_block[i] != 0)
	{
		// For the ideal length
		reqLen = 4 * ((8+ strlen(name) + 3) / 4);
		// get the data blocks of the parent
		get_block(dev, locInoPtr->i_block[i], buf);
		cp = (char *)buf;

		// remaining is the last entrys rec_len minus its ideal len
		while(cp < (buf + BLOCK_SIZE))
		{
			// we keep going until we have the last entry with the required len
			locDirPtr = (DIR *)cp;
			prev_cp = cp;
			cp += locDirPtr->rec_len;
		}

		// We calc the remaining free space needed
		freeSpace = locDirPtr->rec_len - (4 * ((8 + locDirPtr->name_len + 3) / 4));

		// avalible data block has enough free space
		if(reqLen <= freeSpace)
		{
			// The new entry is entered as the last entry.

			// rec_len is set to the ideal length
			locDirPtr->rec_len = (4 * ((8 + locDirPtr->name_len + 3) / 4));
			// Set prev cp pointer to the new entry
			prev_cp += locDirPtr->rec_len;

			// The last entry of the datablock is modified
			locDirPtr = (DIR *)prev_cp;
			locDirPtr->inode = parentMinoPtr->ino;
			locDirPtr->rec_len = freeSpace;
			locDirPtr->name_len = strlen(name);
            locDirPtr->file_type = EXT2_FT_DIR;            // EXT2_FT_DIR = 2 declared in ext2_fs.h

			strcpy(locDirPtr->name, name);

			// Write the data block to disk
			put_block(dev, locInoPtr->i_block[i], buf);

		}
		// Need to create a new data block
		else
		{
			idx = GetNotFullIblockIndex(parentMinoPtr, name);
			// Allocate a new data block
            tempBlockNum = balloc(dev);
			parentMinoPtr->INODE.i_block[idx] = tempBlockNum;

            // INC parent's size by 1024;
            parentMinoPtr->INODE.i_size += BLKSIZE;

            //  Enter new entry as the first entry in the new data block
            //    with rec_len=BLKSIZE.
            get_block(dev, tempBlockNum, buf);
            newDirPtr = (DIR *)buf;
            newDirPtr->inode = parentMinoPtr->ino;
            newDirPtr->rec_len = BLKSIZE;
            newDirPtr->name_len = strlen(name);
            locDirPtr->file_type = EXT2_FT_DIR;            // EXT2_FT_DIR = 2 declared in ext2_fs.h
            strcpy(newDirPtr->name, name);

            // Write data block to disk
            put_block(dev, tempBlockNum, buf);
            break;
		}


		i++;
	}

}

DIR *find_last_entry(char *buf)
{
    DIR *dp = (DIR *)buf;
    char *cp = buf;

    printf("Step to last entry\n");
    int i = 0;
    char *temp;
    while(cp + dp->rec_len < buf + BLKSIZE)
    {
        if(dp->rec_len <= 0)
        {
            printf("Error: last entry invalid record length\n");
            return 0;
        }

        temp = strdup(dp->name);
        temp[dp->name_len] = 0;
        // print DIR entries to see what they are
        printf("%d. %s\n", i++, temp);

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }

    return dp;
}

// Returns the index of a useable (has enough space for dir entry) i block.
int GetNotFullIblockIndex(MINODE *mip, char *name)
{
	int index = 1, BlockNum, NeedLength, RemainingSpace;
    char *cp;
    char buf[BLKSIZE];
    INODE *LocalInodePtr = &mip->INODE;
    DIR *LocalDirPtr;
    NeedLength = 4 * ((8 + strlen(name) + 3) / 4);

    while (index < 14)
    {
        // We are looking at a totally empty i_block[] index, so we break out of the loop
        // and return the index
        if(LocalInodePtr->i_block[index] == 0)
        {
            break;
        }

        BlockNum = LocalInodePtr->i_block[index];
        get_block(dev, BlockNum, buf);
        cp = (char *)buf;

        while (cp < (buf + BLKSIZE))
        {
            LocalDirPtr = (DIR *)cp;
            cp += LocalDirPtr->rec_len;
        }

        RemainingSpace = LocalDirPtr->rec_len - ( 4 * ((8 + LocalDirPtr->name_len + 3) / 4) );

        // There are entries in this i_block[] index, but there is enough space to
        // add this entry
        if(RemainingSpace > NeedLength)
        {
            break;
        }

		index++;
    }
    return index;
}



//Mo: Prints the cwd using the running proc pointer
int pwd(char *pathstr)
{
    printf("INSIDE PWD()\n");
	char temp_name[128];
	char temp_path[256];
	char path[256];
	int ino_num, parent_ino_num;
	MINODE *LocalMinoPrntPtr, *LocalMinoPtr;

	strcpy(path, "");

	LocalMinoPtr = running->cwd;

	findinode(LocalMinoPtr, &ino_num, &parent_ino_num);


	while(parent_ino_num != ino_num)
	{
		// test
        printf("PINoN = %d, LinoN = %d\n", parent_ino_num, ino_num);

		LocalMinoPrntPtr = (MINODE *)iget(dev, parent_ino_num);

		// test
        printf("before get ino name\n");
		getinoname(LocalMinoPrntPtr, ino_num, temp_name);


		// test
		printf("outside get inode name function\n");

		strcpy(temp_path, path);
        strcpy(path, temp_name);
        strcat(path, "/");
        strcat(path, temp_path);
		findinode(LocalMinoPrntPtr, &ino_num, &parent_ino_num);
	}
	strcpy(temp_path, path);
	strcpy(path, "/");
    strcat(path, temp_path);

    printf("%s\n", path);
    return 0;
}


int rm_dir(char *path)
{
    char *parent, *child, *dirc, *basec;


    uint32_t ino;

    MINODE *pip;            // parent Inode pointer
    MINODE *mip;            // In Memory Inode pointer
    INODE *parent_inode;

     // check if path of the directory to be removed is given
    if(path[0] == NULL)
    {
        perror("You must specify a pathname\n");
        return -1;
    }

    printf("Get the ino of %s\n", path);
    // get the inode number
    ino = getino(dev, path);
    printf("rmdir %s ino = %d\n", path, ino);
    // check its a valid inode number
    if(ino <= 0)
    {
        perror("Invalid! pathname not found\n");
        return -1;
    }

    // get the minode (In memory inode from MINODE [] )
    mip = iget(dev, ino);

    // check the i_mode if its not a directory then error
    if(!S_ISDIR(mip->INODE.i_mode))
    {
         // put the inode back
        iput(mip->dev, mip);
        return -1;
    }

    // Check the refCount of the inode to make sure directory is not in use
    if(mip->refCount > 1)
    {
        perror("Error: directory is in use\n");
        return -1;
    }

    // Check to make sure directory is empty
    if(mip->INODE.i_links_count > 2)
    {
        perror("Error: directory is not empty\n");
        return -1;
    }

    // Check datablocks of directory for files
    if(dir_isempty(mip) > 0)
    {
        perror("Error: directory is not empty\n");
    }


    // Deallocate datablocks 0-11
    int i = 0;
    while(i < 12)
    {
        if(mip->INODE.i_block[i])
        {
            printf("deallocating datablock : i_block[%d]....\n", i);
            bdealloc(mip->dev, mip->INODE.i_block[i]);
        }
        i++;
    }

    // Deallocate the inodes
    idealloc(mip->dev, mip->ino);

    // Get Basename and Dirname of from path
    // if directory is /a/b/c the parent = /a/b child = c or if directory a/b/c then parent is a/b and child is c
    // if command is mkdir C then parent is . and child is C
    dirc = strdup(path);
    basec = strdup(path);
    parent = dirname(dirc);
    child = basename(basec);

    printf("rmdir: parent(dirname) = %s, child(basename)= %s\n", parent, child);

    // Get ino of parent
    ino = getino(mip->dev, parent);
    printf("parent ino = %u\n", ino);

    // get the parent inode pointer
    pip = iget(mip->dev, ino);
    iput(mip->dev, mip);

    // Remove the child (basename): the directory to be removed
    rm_child(pip, child);

    // Decrease the parent Inode linkcount by 1
    pip->INODE.i_links_count--;

    // Update parent Inode information and set dirty =1 (modified)
    touch_update (parent);
    pip->dirty = 1;

    // Put the parent Inode back into disk
    iput(pip->dev, pip);

    return 0;

}
int touch_update(char *path)
{
    char buf[BLOCK_SIZE];
    int ino;
    MINODE *mip;

    // Step 1: Get ino of path
    ino = getino(dev, path);
    printf("touch_update dev = %d ino = %d\n", dev, ino);
    getchar();

    // Case 0: Ino < 0 we want to create a file
    if(ino <= 0)
    {
        // Create_file() function goes here
        return 0;
    }

    // Step 2: update
    // Case 1: Ino > 0 we want to update file or directory
    // Get the IN_Memory Inode of path
    mip = iget(dev, ino);
    mip->INODE.i_atime = mip->INODE.i_mtime = mip->INODE.i_ctime = time(0L);

    // Step 3: Set mip->dirty
    mip->dirty = 1;

    // Step 4: Put the In_Memory Inode back into disk
    iput(mip->dev, mip);

    return 0;
}

int rm_child(MINODE *pip, char *child)
{
    printf("rm_child() directory: %s\n", child);
    char *cp, *cp2;        // pcp: previous char pointer, ccp : current char pointer
    DIR *dp, *dp2, *prevdp;          // pdp: previous dir entry *, cdp: current dir entry*

    char buf[BLOCK_SIZE], buf2[BLOCK_SIZE];  // current buffer
    char dirnametmp[256];
    bzero(buf2, BLOCK_SIZE);

    INODE *tmpIp = &pip->INODE;
    int i = 0, found_child = -1;

    // Search the datablocks
    for(i = 0; i < 12; i++)
    {
        if(tmpIp->i_block[i] == 0)
        {
            printf("rmchild(): i_block[%d] is empty\n", i);
            return 0;
        }

        // Read in the i_block[i] into the buffer
        get_block(pip->dev, tmpIp->i_block[i], buf);
        dp      = (DIR *)buf;
        dp2  = (DIR *)buf;
        prevdp  = (DIR *)buf;

        cp      = buf;
        cp2     = buf;

        // Step through the entries inside directory

        printf("Stepping through dir entries iblock = %d\n", i);
        while(cp < (buf + BLOCK_SIZE) && (found_child < 0))
        {

            char *temp = malloc(sizeof(char *) * dp->name_len + 1);
            strncpy(temp, dp->name, dp->name_len );
            temp[dp->name_len + 1] = 0;

            printf("%s\n", temp);
            printf("********************\n");
            if(strcmp(child, temp) == 0)
            {
                printf("found child in i_block[%d]\n", i);
                getchar();
                // IS Child the only entry in the i_block
                if(cp == buf && dp->rec_len == BLOCK_SIZE)
                {
                    printf("%s is the only entry in i_block[%d\n", child, i);

                    // deallocate the i_block, seat i_block[i] = 0 and dcrease the i_blocks count by 1
                    bdealloc(pip->dev, tmpIp->i_block[i]);
                    tmpIp->i_block[i] = 0;
                    tmpIp->i_blocks--;
                    found_child = i;

                    printf("found child in i_block #%d\n", found_child);
                    put_block(pip->dev, pip->INODE.i_block[found_child], buf);
                    getchar();

                    free(temp);
                    return 0;
                }
                else
                {
                    printf("%s is not the only entry in the block\n", child);

                    // Travers through the directory entries
                    printf("finding last entry");
                    while((dp2->rec_len + cp2) < buf + BLOCK_SIZE)
                    {


                        prevdp = dp2;
                        strncpy(dirnametmp, prevdp->name, dp->name_len );
                        dirnametmp[dp->name_len + 1] = 0;
                        printf("%s\n", dirnametmp);

                        cp2 += dp2->rec_len;
                        dp2 = (DIR *)cp2;
                        getchar();
                    }

                    if(dp == dp2)
                    {
                        printf("%s is last entry in block\n", child);
                        prevdp->rec_len += dp->rec_len;

                        found_child = i;
                        printf("found child in i_block #%d\n", found_child);
                        put_block(pip->dev, pip->INODE.i_block[found_child], buf);
                        getchar();

                        return 0;
                    }
                    else
                    {

                        found_child = i;

                        printf("%s is not the last entry in block\n\n", child);
                        int size = (buf + BLOCK_SIZE) - (cp + dp->rec_len);
                        printf("Size of previous record entry = %d\n", size);
                        dp2->rec_len += dp->rec_len;

                        printf("dp2->rec_lec = %d\n", dp2->rec_len);
                        memmove(cp, (cp + dp->rec_len), size);
                        prevdp = (DIR *)cp;
                        printf("previous dir name: temp = %s\n", prevdp->name);

                        printf("found child in i_block #%d\n", found_child);
                        put_block(pip->dev, pip->INODE.i_block[found_child], buf);

                        getchar();
                        return 0;
                    }

                }
            }

            // Go to the next entry by adding the record length
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

    }

    return 0;
}

int dir_isempty(MINODE *mip)
{
    DIR *dp;                    // directory pointer contains points to direntries
    char *cp;                   // points to a single directory entry
    char buf[BLOCK_SIZE];       // holds contents of the block

    int i = 0;

    // First data blocks i_block[0] - i_block[11]
    while(i < 12)
    {
        if(mip->INODE.i_block[i] == 0)
        {
            printf("directory is empty");
            return 0;                   // directory is empty
        }
        // Get the block information

        get_block(mip->dev, ip->i_block[i], buf);
        dp = (DIR*)buf;
        cp = buf;

        char *tmp;
        while(cp < buf + BLOCK_SIZE)
        {
            tmp = strdup(dp->name);
            printf("found entry: %s\n", tmp);

            if(strncmp(".", tmp, 1) && strncmp("..", tmp, 2))
            {
                printf("Found entries besides . .. there must be a link somewhere\n");
                return 1;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        i++;
    }
	return 0;
}




int rm_file(char *path)
{
    char *parent, *child, *dirc, *basec;

    uint32_t ino;

    MINODE *pip;            // parent Inode pointer
    MINODE *mip;            // In Memory Inode pointer
    INODE *parent_inode;

     // check if path of the directory to be removed is given
    if(path[0] == NULL)
    {
        perror("You must specify a pathname\n");
        return -1;
    }

    printf("Get the ino of %s\n", path);
    // get the inode number
    ino = getino(dev, path);
    printf("rm %s ino = %d\n", path, ino);
    // check its a valid inode number
    if(ino <= 0)
    {
        perror("Invalid! pathname not found\n");
        return -1;
    }

    // get the minode (In memory inode from MINODE [] )
    mip = iget(dev, ino);

    // check the i_mode if its not a directory then error
    if(!S_ISREG(mip->INODE.i_mode))
    {
        printf("Error: not a file\n");
         // put the inode back
        iput(mip->dev, mip);
        return -1;
    }

    // Check the refCount of the inode to make sure directory is not in use
    if(mip->refCount > 1)
    {
        perror("Error: file is in use\n");
        return -1;
    }

    // Deallocate datablocks 0-11
    int i = 0;
    while(i < 12)
    {
        if(mip->INODE.i_block[i])
        {
            printf("deallocating datablock : i_block[%d]....\n", i);
            bdealloc(mip->dev, mip->INODE.i_block[i]);
        }
        i++;
    }

    // Deallocate the inodes
    idealloc(mip->dev, mip->ino);

     // Get Basename and Dirname of from path
    // if directory is /a/b/c the parent = /a/b child = c or if directory a/b/c then parent is a/b and child is c
    // if command is mkdir C then parent is . and child is C
    dirc = strdup(path);
    basec = strdup(path);
    parent = dirname(dirc);
    child = basename(basec);

    printf("dirname = %s basename = %s\n", parent, child);
    getchar();


    // Get ino of parent
    ino = getino(mip->dev, parent);
    printf("parent ino = %u\n", ino);

    // get the parent inode pointer
    pip = iget(mip->dev, ino);
    iput(mip->dev, mip);

    // Remove the child (basename): the directory to be removed
    rm_child(pip, child);

    // Decrease the parent Inode linkcount by 1
    pip->INODE.i_links_count--;

    // Update parent Inode information and set dirty =1 (modified)
    touch_update (parent);
    pip->dirty = 1;

    // Put the parent Inode back into disk
    iput(pip->dev, pip);




}


int my_chmod(char *path)
{
    char permMode[64], mPath[256];
    uint32_t ino, mode;
    MINODE *mip;
    char *temp;

    // Write 0's inside the permissions array
    bzero(permMode, 64);
    temp = strchr(path, " ");

    // Step 1: Permissions are stored in the global variable params
    if(params[0] == NULL)
    {
        printf("Usage: chmod [path] [permissions]\n");
        return 0;
    }

    printf("Change access permissions: path = %s mode = %s \n", path, params);

    // Step 2: change permissions to ulong
    mode = strtoul(params, NULL, 8);
    printf("mode = %u\n", mode);

    // Step 3: Get ino of path
    ino = getino(dev, path);
    if(ino <= 0)
    {
        printf("Invalid path error\n");
        return -1;
    }

    // Step 4: get the IN_Memory inode
    mip = iget(dev, ino);
    printf("chmod: mip->dev = %d mip->ino = %d\n", mip->dev, mip->ino);


    int i = i = ~0x1FF;
    printf("i = %d\n", i);

    mip->INODE.i_mode &= i;
    printf("mip->INODE.i_mode &= i: %d\n", mip->INODE.i_mode);

    mip->INODE.i_mode |= mode;
    printf("mip->INODE.i_mode |= i: %d\n", mip->INODE.i_mode);

    // Step 5: Set dirty bit to 1 to indicate its been modified
    mip->dirty = 1;

    // Step 6: write changes to disk
    iput(mip->dev, mip);

    getchar();

    return 1;
}

int split_path(char *original, char *path1, char *perm)
{
    char *token;

    token = strtok(original, " ");
    strcpy(path1, token);

    // Get the permission string from path
    token = strtok(0, " ");
    if(token == NULL)
    {
        printf("Error missing permission: chmod [path] [permission]\n");
        return -1;
    }

    strtok(perm, token);
    return 1;




}























