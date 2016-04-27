#include "init.h"


MINODE      MemoryInodeTable[NMINODES];
MOUNT       MountTable[NMOUNT];
PROC        ProcessTable[NPROC];
OFT         OpenFileTable[NOFT];



MINODE      *root;
PROC        *running;
PROC        *usrProcP1;

int dev;
int inodeBegin;
int bmap;
int imap;
int ninodes;
int nblocks;
char pathname[128];




void init_fs()
{
    printf("Initializing FileSystem.....\n");

    /*  (1). 2 PROCs, P0 with uid=0, P1 with uid=1, all PROC.cwd = 0
        (2). MINODE minode[100]; all with refCount=0
        (3). MINODE *root = 0;
    */
    PROC    *pp  = NULL;           // Process pointer saves typing
    OFT     *oft = NULL;          // pointer to openfile table
    MINODE  *mip = NULL;
    MOUNT   *mp  = NULL;

    int i;
    for(i = 0; i < NPROC; i++)
    {
        pp = &ProcessTable[i];

        // Set each process pip: uid = 0, gid = 0, wd = 0,
        pp->pid = i;
        pp->uid = 0;
        pp->gid = 0;
        pp->cwd = 0;
        pp->status = FREE;

        // Set each process next pointer to NULL
        pp->next   = NULL;

        int j = 0;
        // Each process has a fd[NFD] array set it to NULL
        while(j < NFD)
        {
            pp->fd[j++] = NULL;
        }
    }

    running     = &ProcessTable[0];
    usrProcP1   = &ProcessTable[1];

    putchar('\n');
    printf("running  process: pid = %d, uid = %d, gid = %d\n", running->pid, running->uid, running->gid);
    printf("userproc process: pid = %d, uid = %d, gid = %d\n\n", usrProcP1->pid, usrProcP1->uid, usrProcP1->gid);

    // Initialize MINODE MemoryInodeTable[NMINODES]: set all refCount = 0 and ino = 0
    i = 0;
    while(i < NMINODES)
    {
        mip = &MemoryInodeTable[i++];
        mip->refCount   = 0;
        mip->ino        = 0;
    }

    printf("%d MemoryInodeTable[] initialized successfully\n", i);

    // Initialize the MOUNT MountTable[NMOUNTS]
    i = 0;
    while(i < NMOUNT)
    {
        mp = &MountTable[i++];
        mip->dev = 0;
    }
    printf("%d MountTable[] initialized successfully\n\n", i);

    // Set MINODE *root to NULL
    root = NULL;


    return 0;

}

int MountRoot(char *device)
{
    char buf[BLOCK_SIZE];           // Block read buffer.
    MOUNT *mp;

    printf("Mounting Root device %s......\n", device);

    // Step 1: Open disk for RDWR
    dev = open(device, O_RDWR);
    if(dev < 0)
    {
        perror("open device");
        exit(EXIT_FAILURE);
    }
    printf("device with name: %s opened successfully dev = %d. \n\n", device, dev);

    // Step 2: Read the superblock into buffer
    get_super_block(dev, buf);
    sp = (SUPER *)buf;

    // check if device is an ext2 file system
    if(check_ext2(sp) < 0)
    {
        printf("check_ext2: %s not and ext2 filesystem", device);
        exit(EXIT_FAILURE);
    }

    /* Store globals: number of inodes from superblock to global variable */
    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;

    bzero(buf, BLOCK_SIZE);
    // Step 3: Read the group descriptor block save  inodeBegin bmap imap to global variable;
    get_gd_block(dev, buf);
    gp = (GD *)buf;

    // Store the globals:
    inodeBegin  = gp->bg_inode_table;
    bmap        = gp->bg_block_bitmap;
    imap        = gp->bg_inode_bitmap;;

    // Step 4: Get root Memory INODE from MemoryInodeTable[]
    root = iget(dev, ROOT_INODE);               // In global.h #define ROOT_INODE    2
    printf("root MemoryMinodeTable: dev = %d ino = %d refCount = %d\n", root->dev, root->ino, root->refCount);

    // Step 5: Use iget to get the MemoryMinode for P0->cwd parent process
    PROC *pp = &ProcessTable[0];
    pp->cwd     = iget(dev, ROOT_INODE);

    // Step 6: Use iget to get the MemoryMinode for P1->cwd child process
    PROC *cpp   = &ProcessTable[1];
    cpp->cwd    = iget(dev, ROOT_INODE);;

    printf("PO MemoryInode: dev = %d ino = %d refCount = %d\n", pp->cwd->dev, pp->cwd->ino, pp->cwd->refCount);
    printf("P1 MemoryInode: dev = %d ino = %d refCount = %d\n", cpp->cwd->dev, cpp->cwd->ino, cpp->cwd->refCount);


    mp = &MountTable[0];

    // Step 7: Assign root Minode to MountTable[0]->mounter_inode and set its number of inodes number of blocks and dev from globals
    mp->mounted_inode = root;
    mp->nblocks = nblocks;
    mp->ninodes = ninodes;
    mp->dev = dev;

    // Step 8: Finally Copy the device name to the MountTable[0]->name
    // be sure to write 256 bytes to fill name with 0's
    strncpy(mp->name, device, 256);

    printf("MountTable[0]: name = %s mounted successfully\n", MountTable[0].name);
    return dev;

}

