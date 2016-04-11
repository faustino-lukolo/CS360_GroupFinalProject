#ifndef _UTILS_H
#define _UTILS_H

#include "type.h"
#include <stdio.h>
#include <stdlib.h>

/************************ globals *****************************/
char pathname[128], parameter[128], *name[128], cwdname[128], blkBuf[BLOCK_SIZE];
int ninodes, nblocks, ifree, bfree, InodeBeginBlock;
char names[128][256];
int iblock;

MINODE *root;
MINODE minode[NMINODES];
MOUNT  mounttab[NMOUNT];
PROC   proc[NPROC], *running;
OFT    oft[NOFT];


int init();
void get_input();
int mount_root(char *dev);

// block operations functions
MINODE *iget(int dev, uint32_t ino);

void get_block(int dev, int blk, char buf[]);
int is_ext2(SUPER *sptr);

#endif
