#ifndef _EXT2_UTILS_H
#define _EXT2_UTILS_H
#include "global.h"


void get_block(int mdev, int blk, char *buf);
int get_gd_block(int dev, char *buf);
int get_super_block(int dev, char *buf);
int check_ext2(SUPER *sp);
MINODE *iget(int mdev, int inumber);


u32 getino(int dev, char *path);
u32 search(int mdev, char *name, INODE *ip);

int put_block(int mdev, int blk, char *buf);
int iput(int mdev, MINODE *mip);

u32 findinode(MINODE *mip, int *myinode, int *parentino);
u32 getinoname(MINODE *parent, int myinode, char *inoname);
u32 searchForParentInode(int mdev, int ino, INODE *ip, char *temp);


/*Inode,Block Alloc/Dealloc Functions*/
void dec_free_inodes(int dev);
void inc_free_inodes(int dev);
void dec_free_blocks(int dev);
void inc_free_blocks(int dev);
int ialloc(int pdev);
int balloc(int pdev);
int idealloc(int dev, int ino);
int bdealloc(int dev, int bno);


int ideal_record_length(const int name_len);
int needed_length(const int name_len);
int findLastBlock(MINODE *pip);

#endif // _EXT2_UTILS_H
