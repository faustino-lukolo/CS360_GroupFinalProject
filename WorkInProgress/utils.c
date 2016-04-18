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
    int ninodes, nblocks, ifree, bfree;

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
    mp->bmap    = gp->bg_block_bitmap;
    mp->imap    = gp->bg_inode_bitmap;


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
        printf("Error: MINODE[] IS FULL!\n");
        return 0;
    }

    // get the inode block iBlock by using mailman's algorithm
    blk = (ino - 1) / 8 + InodeBeginBlock;
    printf("iget() blk = %d\n", blk);

    // get the offset of the inode
    offset = (ino - 1) % 8;
    printf("iget() offset = %d\n", offset);


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

    printf("iget() Working!\n");

    return &minode[i];
}
int iput(int mdev, MINODE *mip)
{
    char buf[BLOCK_SIZE];
    int blk, offset;
    INODE *tmpip;

    printf("iput(): dev = %d\n", mdev);

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
    char **names;               // Holds the tokens of path i.e. /a/b/c/ -> [a][b][c]...
    MINODE *mip = 0;

    if (path[0]) {

        names = tokenize(pathname);     // Break path up into tokens
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

    int i = 0;

    // search for ino of each path in the names[][]
    while (names[i]) {
        printf("searching ino of %s\n", names[i]);
        ino = search(dev, names[i], ip);            // remember: ip = &running->cwd->INODE; where running is a PROC *running

        if(ino <= 0){
            // The path doesnt exits put mip back into the device
            if(mip)
            {
                iput(mip->dev, mip);
                return -1;
            }
        }
        printf("[%s] ino = %d\n", names[i], ino);

        if(mip){
            printf("put mip back into device\n");
            iput(mip->dev, mip);
        }
        i++;
        printf("get the mip for: name = %s device = %d ino = %d\n", names[i], dev, ino);
        if(names[i])
        {
            mip = iget(dev, ino);
            ip = &mip->INODE;
        }

    }

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
    char **pathArr = (char **)malloc(sizeof(char *) * 256);

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
    printf("cmd = ls path = %s \n", pathname);
    getchar();

    /* Case 1: pathname is null set inode number to the running->cwd->ino
    // The Inode we are interested in is the current running process inode number
       Case 2: pathname begins with / and  pathname[1] is black
       Case 3: pathname is other than just / i.e. /a/b/c a/b/c then we have to find the ino
    */
    if(!path || !pathname[0])
    {
        ino = running->cwd->ino;
        printf("running->cwd->ino = %d\n", ino);
    }
    else if(pathname[0] == '/' && pathname[1] == 0)
        ino = root->ino;
    else
        ino = getino(dev, path);

    if(!ino)
    {
        printf("Invalid pathname\n");
        return -1;
    }

    // // Get the indoe from minode[]
    mip = iget(dev, ino);
    printf("ls() dev = %d ino = %d\n", mip->dev, ino);

    // // Each data block of mip->INODE contains DIR entries
    findDatablocks(&mip->INODE, 0);
    iput(mip->dev, mip);
    getchar();

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


// Finds the datablocks where the dir entries are inside minode[]
int findDatablocks(INODE *ip, int pstat)
{
    int i;
    uint32_t buf[256], mbuf[256];
    // Search the first 11 blocks : Direct blocks
    for (i = 0; i < 12; i++) {
        if (ip->i_block[i]) {
            printDirEntry(ip->i_block[i], pstat);
        }
    }

    // Search i_blocks[12]: indirect blocks
    if(ip->i_block[12])
    {
        // read indirect block into buffer
        get_block(dev, ip->i_block[12], (char*)buf);

        // there are 256 blocks in i_block[12] indirect block
        for (i = 0; i < 256; i++) {
            if (buf[i]) {
                printDirEntry(buf[i], pstat);
            }
        }
    }

    // Search i_block[13]: double indirect blocks
    if (ip->i_block[13]) {

        // read the block into buffer
        get_block(dev, ip->i_block[13], (char*)buf);

        int j;
        // there are 256 blocks in i_block[13] each of which has 256 additional blocks
        for (i = 0; i < 256; i++) {
            if (buf[i]) {
                // read this block into mbuf
                get_block(dev, buf[i], (char*)mbuf);
                // search the 256 blocks of block buf[i]
                for (j = 0; j < 256; j++) {
                    if(mbuf[j])
                    {
                        printDirEntry(mbuf[j], pstat);
                    }
                }
            }
        }
    }

    return 0;
}
int printDirEntry(int blk, int pstat)
{
    int i;
    char *cp;
    DIR *dp;
    char mbuf[BLOCK_SIZE], tmpname[256];

    get_block(dev, blk, mbuf);
    dp = (DIR*)mbuf;
    cp = mbuf;

    // Search the buffer for all dir entries
    while (cp < mbuf + BLOCK_SIZE) {
        if (pstat) {
            printf("print file stat... \n");
        }
        else
        {
            // Clear the temp dir entry name
            // copy the dp->name found in DIR Struct into tmpname
            bzero(tmpname, 256);
            strncpy(tmpname, dp->name, dp->name_len);
            printf("%4d %4d %4d %s\n", dp->inode, dp->rec_len, dp->name_len, tmpname);
        }

        // Go to the next entry by adding the rec_len to the char pointer cp
        cp += dp->rec_len;
        // set the Dir pointer dp to this next entry and repeat
        dp = (DIR *)cp;
    }
    return 0;
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
    char parent[256], child[256], pathnameCopy[512];
    MINODE *mip;
    int mdev, ino;

    printf("mkdir(): path = %s\n", path);

    // Clear the arrays
    bzero(parent, 256);
    bzero(child, 256);
    bzero(pathnameCopy, 512);

    // preserve the pathname
    strcpy(pathnameCopy, path);
    if(path[0] == '/')
    {
        mdev = root->dev;
        printf("path starts from: root device = %d\n", mdev);
    }
    else{
        mdev = running->cwd->dev;
        printf("path starts running process cwd: running->cwd->dev = %d\n", mdev);
    }


    return 0;
}

// Prints the cwd using the running proc pointer
int pwd(char *pathstr)
{
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
		printf("outside get inode name function");
		
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











































