#ifndef TYPE_H
#define TYPE_H
/*	type.h for CS360 Project             */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
/* ext2_inode struct & members
struct ext2_inode {
  //*************************************************************************
  u16  i_mode;          // same as st_mode in stat() syscall
  u16  i_uid;                     // ownerID
  u32  i_size;                    // file size in bytes
  u32  i_atime;                   // time fields
  u32  i_ctime;
  u32  i_mtime;
  u32  i_dtime;
  u16  i_gid;                     // groupID
  u16  i_links_count;             // link count
  u32  i_blocks;                  // IGNORE
  u32  i_flags;                   // IGNORE
  u32  i_reserved1;               // IGNORE
  u32  i_block[15];               // See details below
  //**************************************************************************
*/
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs
/* ext2_dir_entry_2 struct & members
struct ext2_dir_entry_2 {
u32 inode;    //inode number; count from 1, NOT 0
u16 rec_len;  // this entry's length in bytes
u8 name_len;  // name length in bytes
u8 file_type; //  not used
char name[EXT2_NAME_LEN];
};*/



//not used
//name: 1 -255 chars, no NULL byte

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)
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
  INODE INODE;               // disk inode, compile knows about struct name format
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



#endif
