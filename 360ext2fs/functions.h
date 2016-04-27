#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H
#include "global.h"


/* Start of Linux Commands Functions */
int ls(char *path);
int cd(char *path);
int make_dir(char *path);
int rm_dir(char *path);
int pwd(char *pathstr);
int creat_file(char *path);
int rm_file(char *path);
int my_chmod(char *path);
int touch_update(char *path);

int quit(char *path);
// Helpers
int ls_dir(MINODE *mip);
int ls_file(MINODE *mip, char *fname);

int addLastBlock(MINODE *pip, int bnumber);

/*Function pointers for commands */
static int (*fptr[])(char*) = {(int (*)())ls, cd, make_dir, creat_file, touch_update, rm_dir, rm_file, pwd, my_chmod, quit};
static char *sh_cmds[] = {"ls", "cd", "mkdir", "creat", "touch", "rmdir","rm" ,"pwd", "chmod", "quit"};
#endif // _FUNCTIONS_H
