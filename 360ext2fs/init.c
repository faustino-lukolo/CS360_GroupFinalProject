#include "global.h"



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

void MountRoot(char *device)
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
    getSuper(dev, buf);
    sp = (SUPER *)buf;

    if(check_ext2(sp) < 0)
    {
        printf("check_ext2: %s not and ext2 filesystem", device);
        exit(EXIT_FAILURE);
    }

    /* The device is an ext2 file system save number of inodes from superblock to global variable */
    ninodes = sp->s_inodes_count;


}



void get_block(int mdev, int blk, char *buf)
{
    lseek(mdev, blk * BLOCK_SIZE, SEEK_SET);
    read(mdev, buf, BLOCK_SIZE);
}

void get_iTable(int mdev)
{
    char buf[BLOCK_SIZE];
    getGDBLOCK(int mdev)

}
void getSuper(int mdev, char *buf)
{
    get_block(mdev, SUPERBLOCK, buf);
}

int check_ext2(SUPER *sp)
{
    if(sp->s_magic != SUPER_MAGIC)
        return -1;

    return 1;
}
