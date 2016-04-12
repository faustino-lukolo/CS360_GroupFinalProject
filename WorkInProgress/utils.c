#include "utils.h"

/* Globals
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
    int i, ino, dev;
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
/*
This is the most important function of the FS. It converts a pathname, such as
 /a/b/c/d OR x/y/z, into its inode number (ino).

 THIS IS THE SAME AS YOUR showblock.c program. Instead of printing blocks,
 return its inode number.
*/
uint32_t getino(int *dev, char *pathname)
{
    uint32_t ino = 0;
    char **tokensArray;
    MINODE *mip;

    if (pathname) {
        tokensArray = tokenize(pathname);
    }

    int i = 0;
    while (tokensArray[i]) {
        printf("path %d = %s\n", i, tokensArray[i++]);
    }
}
/*
  tokenize a pathname into components and their numbers n.
  Store the components in names[64][64] and let name[i] point at names[i];
  The components will be used to search for the inode of a pathname.
*/
char ** tokenize(char *pathname)
{

    int i = 0;
    char *token, *pathCopy;            // Holds a path name
    // A 2Dimensional Array char *names[256]
    char **names = (char **)malloc(sizeof(char *) * 256);

    // preserve the pathname
    pathCopy = strdup(pathname);

    // Get the first token from the pathname
    names[i++] = strtok(pathCopy, "/");
    while (names[i] = strtok(NULL, "/") != NULL) {
    i++;
    }

    names[i] = 0;

    // rest counter
    i = 0;
    while (names[i]) {
        token = (char*)malloc(sizeof(char) * strlen(names[i]));
        strcpy(token, names[i]);
        names[i] = token;
        i++;
    }

    return names;
}
void get_input()
{
    printf("commands: [cd ls]\n");
    printf("cmd: ");

    fgets(line, 256, stdin);
    if (line[strlen(line) -1] == '\n') {
        /* code */
        printf("newline found\n");
        line[strlen(line) -1] = 0;
    }
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
    int ino;                // the inode number of path
    MINODE *mip;            // pointer to the inode inside minode[] array
    printf("cmd = ls path = %s \n", pathname);
    getchar();

    // Case 1: pathname is null set inode number to the running->cwd->ino
    // The Inode we are interested in is the current running process inode number
    if(!path || !pathname[0])
    {
        ino = running->cwd->ino;
        printf("running->cwd->ino = %d\n", ino);
    }

    getchar();



}
