#ifndef _GLOBAL_H
#define _GLOBAL_H

/*	type.h for CS360 Project             */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)   // Shortens memset() function for zeroing out arrays




// define shorter TYPES, save typing efforts
typedef unsigned char  u8;            // unsigned char
typedef unsigned short u16;           // unsigned short
typedef unsigned int   u32;           // unsigned int


typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs


GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;

#define BLOCK_SIZE        1024
#define BLKSIZE           1024
#define BITS_PER_BLOCK    (8*BLOCK_SIZE)
#define INODES_PER_BLOCK  (BLOCK_SIZE/sizeof(INODE))

// Block number of EXT2 FS on FD
#define SUPERBLOCK        1
#define GDBLOCK           2
#define BBITMAP           3
#define IBITMAP           4
#define INODEBLOCK        5
#define ROOT_INODE        2

// Default dir and regulsr file modes
#define DIR_MODE          0040777
#define FILE_MODE         0100644
#define SUPER_MAGIC       0xEF53
#define SUPER_USER        0

// Proc status
#define FREE              0
#define BUSY              1
#define KILLED            2

// Table sizes
#define NMINODES          50
#define NMOUNT            10
#define NPROC             10
#define NFD               10
#define NOFT              50

// Open File Table
typedef struct Oft{
  int   mode;
  int   refCount;
  struct Minode *inodeptr;
  long  offset;
} OFT;

// PROC structure
typedef struct Proc{
  int   uid;
  int   pid;
  int   gid;
  int   ppid;
  int   status;

  struct Minode *cwd;
  OFT   *fd[NFD];

  struct Proc *next;
  struct Proc *parent;
  struct Proc *child;
  struct Proc *sibling;
} PROC;

// In-memory inodes structure
typedef struct Minode{
  INODE INODE;               // disk inode
  int   dev, ino;

  int   refCount;
  int   dirty;
  int   mounted;
  struct Mount *mountptr;
  char     name[128];           // name string of file
} MINODE;

// Mount Table structure
typedef struct Mount{
  int  ninodes;
  int  nblocks;
  int  bmap;
  int  imap;
  int  iblock;
  int  dev, busy;
  struct Minode *mounted_inode;
  char   name[256];
  char   mount_name[64];
} MOUNT;

/* More global declarations */
typedef enum { false, true } bool;

enum stat_mode
{
   MODE_REG = 0b1000,
   MODE_DIR = 0b0100,
   MODE_LNK = 0b1010
};


// Declared in outside file

// Global Variables
extern int dev;
extern int inodeBegin;
extern int bmap;
extern int imap;
extern int ninodes;
extern int nblocks;


extern char pathname[];     // global pathname


extern MINODE* root;
extern PROC* running; // Points at the PROC structure of the current running process
// Every file operation is performed by the current running process

extern MINODE    MemoryInodeTable[];
extern MOUNT     MountTable[];
extern PROC      ProcessTable[];
extern OFT       OpenFileTable[];





#endif
