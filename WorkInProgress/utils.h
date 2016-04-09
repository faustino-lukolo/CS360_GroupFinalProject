#ifndef _UTILS_H
#define _UTILS_H

#include "type.h"
#include <stdio.h>
#include <stdlib.h>

/************************ globals *****************************/
MINODE *root;
char pathname[128], parameter[128], *name[128], cwdname[128];
char names[128][256];
int iblock;

MINODE minode[NMINODES];
MOUNT  mounttab[NMOUNT];
PROC   proc[NPROC], *running;
OFT    oft[NOFT];


int init();
void get_input();
int mount_root(char *dev);

void get_block(int dev, int blk, char buf[]);

#endif
