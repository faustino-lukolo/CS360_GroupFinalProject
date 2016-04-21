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
void get_block(int dev, int blk, char mbuf[]);

MINODE *iget(int dev, uint32_t ino);
int iput(int mdev, MINODE *mip);
int put_block(int mdev, int blk, char *buf);
uint32_t getino(int dev, char *path);
uint32_t search(int mdev, char *name, INODE *ip);
uint32_t getinoname(MINODE *parent, int myinode, char *inoname);
uint32_t findinode(MINODE *mip, int *myinode, int *parentino);

void get_block(int dev, int blk, char buf[]);
int is_ext2(SUPER *sptr);


/* Start of Linux Commands Functions */
int ls(char *path);
int cd(char *path);
int make_dir(char *path);
int pwd(char *pathstr);

// Helpers
int my_mkdir(MINODE *pip, char *bname);



// ext2 utils functions
int findDatablocks(INODE *ip, int pstat);
int printDirs(int blk, int pstat);
int printstat(DIR *dp);

/*Function pointers for commands */
static int (*fptr[])(char*) = {(int (*)())ls, cd, make_dir, pwd};
static char *sh_cmds[] = {"ls", "cd", "mkdir", "pwd"};
#endif
