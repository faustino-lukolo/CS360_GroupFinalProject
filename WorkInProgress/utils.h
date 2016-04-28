#ifndef UTILS_H
#define UTILS_H

#include "type.h"
#include <stdlib.h>
#include <time.h>

/************************ globals *****************************/
char cmd[32], pathname[128], params[64], cwdname[128], blkBuf[BLOCK_SIZE], line[1024];
int ninodes, nblocks, ifree, bfree, InodeBeginBlock, nproc, dev, bmap, imap;

int iblock;
int openfd;

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
u32 getino(int dev, char *path);
u32 search(int mdev, char *name, INODE *ip);
u32 getinoname(MINODE *parent, int myinode, char *inoname);
u32 findinode(MINODE *mip, int *myinode, int *parentino);

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
int Link(char *oPath);
int SymLink(char *oPath);
int open_file(char *path);
int mlseek(char *path);
int close_file(char *path);
int read_file(char *path);
<<<<<<< Updated upstream
//int mycat(char *fname);
=======
int my_chown(char *path);
int my_chmod(char *path);
int my_unlink(char *path);
int quit(char *path);
>>>>>>> Stashed changes

// BLOCK Operations
int get_super_block(int dev, char *buf);


// Helpers
int my_mkdir(MINODE *pip, char *bname);
void PutNamePDir(MINODE *parentMinoPtr, int ino, char *name);
int GetNotFullIblockIndex(MINODE *mip, char *name);
int my_creat(MINODE *pip, char *name);
char *read_link(char *path);
int TruncateFileMino(MINODE *myMinoPtr);
int ls_dir(MINODE *mip);
int ls_file(MINODE *mip, char *file);
int rm_file(char *path);
char *read_link(char *path);
int pfd(); // disply currently opened files.
int myread(int fd, char *buf, int nbytes);
int getMinBytes(int nbytes, int avilbytes, int remainbytes);

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
#define NUM_CMDS 17
static int (*fptr[])(char*) = {(int (*)())ls, cd, make_dir, creat_file,rm_dir, rm_file, pwd, Link, my_unlink, SymLink, my_chown, my_chmod, open_file, mlseek, close_file, read_file, quit};
static char *sh_cmds[] = {"ls", "cd", "mkdir", "creat", "rmdir", "rm", "pwd", "ln", "ul", "symlink", "chown", "chmod", "open", "lseek", "close", "read", "quit"};

#endif
