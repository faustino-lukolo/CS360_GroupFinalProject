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
extern char line[1024];
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

    // Initialize proc[NPROC] set the status to FREE
    for (i = 0; i < NPROC; i++) {
        /* code */
        proc[i].status = FREE;

        // each process has a fd[NFD] file descriptor array. Set all of it to 0
        for (j = 0; j < NFD; j++) {
            /* code */
            proc[i].fd[j] = 0;
        }

        // Finally link every process to its neighbor. The proccess are essentially linked list
        proc[i].next = &proc[i +1];
    }

    // Initialize oft[NOFT] open file Table. Set the refCount = 0
    for (i = 0; i < NOFT; i++) {
        /* code */
        oft[i].refCount = 0;

    }
    printf("INIT OK!\n");
    mount_root(rootdev);

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

    // Setup mounttab[0] this will be our root device and will contain
    // Inodes and block information
    mp = &mounttab[0];

    // save important inode and block information
    /* copy super block info to mounttab[0] */
    ninodes = mp->ninodes = sp->s_inodes_count;
    nblocks = mp->nblocks = sp->s_blocks_count;

    // save the free inodes and free blocks
    ifree = sp->s_free_inodes_count;
    bfree = sp->s_free_blocks_count;

    // printf("nblock count  = %d\n", nblocks);
    // printf("ninodes count = %d\n", ninodes);
    // printf("free inodes   = %d\n", ifree);
    // printf("free blocks   = %d\n", bfree);


    // Get Block 2 Group descriptor Block
    get_block(dev, GDBLOCK, buf);
    gp = (GD *)buf;

    // Assign mounted device to the dev descriptor and set it to busy
    mp->dev = dev;
    mp->busy = BUSY;

    // Copy the group descrip bmap and imap and InodeBeginBlock (Inode Table)
    // into mounttab[0]
    mp->bmap    = gp->bg_block_bitmap;
    mp->imap    = gp->bg_inode_bitmap;

    // Save the inode begin block
    mp->iblock  = InodeBeginBlock = gp->bg_inode_table;

    // copy the root device name into mounttab[0]
    strcpy(mp->name, devName);
    // copy the '/' root to mounttab[0] device name
    strcpy(mp->mount_name, "/");

    // display group descriptor information
    printf("bmap = %d imap = %d InodeBeginBlock (iblock) = %d \n", gp->bg_block_bitmap, gp->bg_inode_bitmap, gp->bg_inode_table);

    /***** call iget(), which inc the Minode's refCount ****/
    root = iget(dev, 2);        // Get root's MINODE inode #2
    mp->mounted_inode = root;   // set the mounttab[0] mounted inode to root

    // print information about the mounted device
    printf("mount: %s mounted on / \n", devName);
    printf("nblocks = %d, bfree = %d ninodes = %d ifree = %d\n", nblocks, bfree, ninodes, ifree);

    return 0;

}

/*
6. MINODE *iget(int dev, int ino)
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


void get_input()
{
    printf("commands: [cd ls]\n");
    printf("cmd: ");

    fgets(line, 1024, stdin);
    if (line[strlen(line) -1] == '\n') {
        /* code */
        printf("newline found\n");
        line[strlen(line) -1] = 0;
    }

}
