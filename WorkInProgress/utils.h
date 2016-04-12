#ifndef UTILS_H
#define UTILS_H

#include "type.h"
#include <stdlib.h>

/************************ globals *****************************/
char cmd[32], pathname[128], params[64], cwdname[128], blkBuf[BLOCK_SIZE], line[256];
int ninodes, nblocks, ifree, bfree, InodeBeginBlock, nproc;

int iblock;

MINODE *root;
MINODE minode[NMINODES];
MOUNT  mounttab[NMOUNT];
PROC   proc[NPROC], *running;
OFT    oft[NOFT];


int init();
void get_input();
char ** tokenize(char *pathname);
int mount_root(char *dev);

// block operations functions
MINODE *iget(int dev, uint32_t ino);
uint32_t getino(int *dev, char *pathname);

void get_block(int dev, int blk, char buf[]);
int is_ext2(SUPER *sptr);


/* Start of Linux Commands Functions */
int ls(char *path);

/*Function pointers for commands */
static int (*fptr[])(char*) = {(int (*)())ls};
static char *sh_cmds[] = {"ls", "cd"};
#endif
