#ifndef UTILS_H
#define UTILS_H

#include "type.h"
#include <stdlib.h>

/************************ globals *****************************/
char cmd[32], pathname[128], params[64], cwdname[128], blkBuf[BLOCK_SIZE], line[1024];
int ninodes, nblocks, ifree, bfree, InodeBeginBlock, nproc, dev;

int iblock;

MINODE *root;
MINODE minode[NMINODES];
MOUNT  mounttab[NMOUNT];
PROC   proc[NPROC], *running;
OFT    oft[NOFT];


int init();
void getInput();
char ** tokenize(char *path);
int mount_root(char *dev);

// block operations functions
MINODE *iget(int dev, uint32_t ino);
int iput(int mdev, MINODE *mip);
int put_block(int mdev, int blk, char *buf);
uint32_t getino(int dev, char *path);
uint32_t search(int mdev, char *name, INODE *ip);

void get_block(int dev, int blk, char buf[]);
int is_ext2(SUPER *sptr);


/* Start of Linux Commands Functions */
int ls(char *path);
int cd(char *path);



// ext2 utils functions
int findDatablocks(INODE *ip, int pstat);
int printDirs(int blk, int pstat);
int printstat(DIR *dp);

/*Function pointers for commands */
static int (*fptr[])(char*) = {(int (*)())ls, cd};
static char *sh_cmds[] = {"ls", "cd"};
#endif
