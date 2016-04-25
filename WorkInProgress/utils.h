#ifndef UTILS_H
#define UTILS_H

#include "type.h"
#include <stdlib.h>
#include <time.h>

/************************ globals *****************************/
char cmd[32], pathname[128], params[64], cwdname[128], blkBuf[BLOCK_SIZE], line[1024];
int ninodes, nblocks, ifree, bfree, InodeBeginBlock, nproc, dev, bmap, imap;

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

/*Inode,Block Alloc/Dealloc Functions*/
void dec_free_inodes(int dev);
void inc_free_inodes(int dev);
void dec_free_blocks(int dev);
void inc_free_blocks(int dev);
int ialloc(int pdev);
int balloc(int pdev);
int idealloc(int dev, int ino);
int bdealloc(int dev, int bno);
int rm_child(MINODE *pip, char *child);


/* Start of Linux Commands Functions */
int ls(char *path);
int cd(char *path);
int make_dir(char *path);
int rm_dir(char *path);
int pwd(char *pathstr);
int creat_file(char *path);

// BLOCK Operations
int get_super_block(int dev, char *buf);


// Helpers
int my_mkdir(MINODE *pip, char *bname);
void PutNamePDir(MINODE *parentMinoPtr, int ino, char *name);
int GetNotFullIblockIndex(MINODE *mip, char *name);
int my_creat(MINODE *pip, char *name); 

// Bit functions
int tst_bit(char *buf, int i);
int set_bit(char *buf, int i);
int unset_bit(char *buf, int i);
int touch_update(char *path);


// ext2 utils functions
int findDatablocks(INODE *ip, int pstat);
int printDirs(int blk, int pstat);
int printstat(DIR *dp);
int dir_isempty(MINODE *mip);


/*Function pointers for commands */
static int (*fptr[])(char*) = {(int (*)())ls, cd, make_dir, creat_file,rm_dir, pwd};
static char *sh_cmds[] = {"ls", "cd", "mkdir", "creat", "rmdir", "pwd"};
#endif
