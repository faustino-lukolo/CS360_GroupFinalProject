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
void get_block(int dev, int blk, char buf[])
{
    // using the device descriptor seek to the block we want to read i.e (blk & 1024)
    lseek(dev, blk * BLOCK_SIZE, SEEK_SET);
    read(dev, buf, BLOCK_SIZE);
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
    mount_root(rootdev);

    printf("INIT OK!\n");

}
int mount_root(char *devName)
{
    // Local variables for file system operations
    int i, ino, fd, dev;
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

    // read super block into buffer
    get_block(dev, SUPERBLOCK, buf);
    sp =(SUPER *)buf;

    is_ext2(sp->s_magic);

    printf("mounting root device = %s\n", devName);
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
