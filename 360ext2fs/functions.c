#include "functions.h"

/* Start of Linux Commands Functions */
int ls(char *path)
{
    /* local variables */
    u32 ino;                // the inode number of path
    MINODE *mip;            // pointer to the inode inside minode[] array
    char *dirc, *basec, *parent, *child;

    //printf("cmd = ls path = %s \n", pathname);
    getchar();

    /* Case 1: pathname is null set inode number to the running->cwd->ino
    // The Inode we are interested in is the current running process inode number
       Case 2: pathname begins with / and  pathname[1] is black
       Case 3: pathname is other than just / i.e. /a/b/c a/b/c then we have to find the ino
    */
    if(!path || !path[0])
    {
        ino = running->cwd->ino;
        //printf("running->cwd->ino = %d\n", ino);
    }
    else if(pathname[0] == '/' && pathname[1] == 0)
    {
        ino = root->ino;
    }
    else
        ino = getino(dev, path);

    if(!ino)
    {
        //printf("Invalid pathname\n");
        return -1;
    }

    // Find the dirname and basenaem
    dirc    = strdup(path);
    basec   = strdup(path);
    parent  = dirname(dirc);
    child   = basename(basec);



    // // Get the indoe from minode[]
    mip = iget(dev, ino);
    printf("ls() dev = %d ino = %d\n", mip->dev, ino);

    // // Each data block of mip->INODE contains DIR entries
    if(S_ISDIR(mip->INODE.i_mode))
    {
       ls_dir(mip);
    }
    else
    {
        ls_file(mip, child);
    }

    iput(mip->dev, mip);
    getchar();

}
int cd(char *path)
{




}
int make_dir(char *path)
{
    char *dirc, *basec, *child, *parent;

    MINODE *mip;
    INODE *pip;
    int mdev, inumber;

    printf("mkdir(): path = %s\n", path);
    mdev = running->cwd->dev;   // start from current working directory

    // Step 1: check if the pathname is absolute
    if(path[0] == '/')
    {
        mdev = root->dev;           // start from root
    }



    // Step 2: get the parent (dirname ) and child (basename)
    // if directory is /a/b/c the parent = /a/b child = c or if directory a/b/c then parent is a/b and child is c
    // if command is mkdir C then parent is . and child is C
    dirc = strdup(path);
    basec = strdup(path);
    parent = dirname(dirc);
    child = basename(basec);

    printf("dirname=%s, basename=%s\n", parent, child);


    // Step 3: Get the parent ino using dev
    inumber = getino(mdev, parent);
    printf("make_dir: inumber = %d\n", inumber);

    // Make sure ino is valid
    if(inumber <= 0)
    {
        printf("Error: path does not exists\n");
        return -1;
    }
    printf("parent ino = %d", inumber);

    // Step 4: Get the In_MEMORY minode of parent:
    mip = iget(mdev, inumber);
    pip = &(mip->INODE);

    // Step 5: Verify parent INODE is a DIR (HOW?)
    if(!S_ISDIR(mip->INODE.i_mode))
    {
        printf("Parent Path is not a directory\n");
        // put inode back into dev
        iput(mdev, mip);
        return -1;
    }

     //Step 6: Verify  child does NOT exists in the parent directory (HOW?);
     // By calling the search function. If the ino returned by the search()
     // then the directory exists
    inumber = search(mdev, child, &(mip->INODE));
    if(inumber > 0)
    {
        printf("ERROR: Directory with name \"%s\" already exists.\n", child);
        // put inode back into dev
        iput(mip->dev, mip);
        return -1;
    }

    getchar();
    printf("make_dir: child = %s ino = %d\n", child, inumber);

    // Step 7: Call my_mkdir() passing parent in_MEMORY minode of parent and the child (basename)
    my_mkdir(mip, child); // mip is parent minode ptr

    iput(mip->dev, mip);
    return 0;

}

// This function handles the dir entry into the MINODE, INODE, DIR fs structures.
// pip points at the parent minode[] of "/a/b", name is a string "c")
int my_mkdir(MINODE *pip, char *name)
{
    int i, RemainingSpace;
    INODE *ip;
    DIR *dp;
	char *cp;


    int InodeNum, BlockNum;
    // Step 1: Allocate free inodes and blocks
    InodeNum = ialloc(pip->dev);
    BlockNum = balloc(pip->dev);


    MINODE *mip;
    // Step 2: get Memory Inode from MemoryMinodeTable inside disk
    mip = iget(pip->dev, InodeNum);
    ip  = &mip->INODE;

    // Step 3: Write directory entry information into the inode
    ip->i_mode = DIR_MODE;
    ip->i_uid  = running->uid;
    ip->i_gid  = running->gid;
    ip->i_size = BLOCK_SIZE;
    ip->i_links_count = 2;
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2;
    mip->dirty = 0;

    // Set i_blocks 0 - 15 = 0
    i = 0;
    while(i < 15)
    {
        ip->i_block[i++] = 0;
    }

    ip->i_block[0] = BlockNum;
    iput(mip->dev, mip);

    // Write . and .. entries
    char buf[BLKSIZE];

    dp        = (DIR *)buf;
    dp->inode = InodeNum;

    strncpy(dp->name, ".", 1);
    dp->name_len = 1;
    dp->rec_len = 12;

    // .. parent directory entry
    cp = buf + 12;
    dp = (DIR *)cp;
    dp->inode = pip->ino;
    dp->name_len = 2;
    strncpy(dp->name, "..", 2);
    dp->rec_len = BLOCK_SIZE - 12;          // take up the rest of the block

    // Write block to disk
    put_block(pip->dev, BlockNum, buf);

    // zero the array
    bzero(buf, BLOCK_SIZE);
    int needLen = needed_length(name);
    BlockNum = findLastBlock(pip);

    get_block(pip->dev, BlockNum, buf);
    cp = buf;
    dp = (DIR *)buf;

    while((dp->rec_len + cp) < buf + BLOCK_SIZE)
    {
        cp += dp->rec_len;
        dp = (DIR *)cp;
    }

    int idealLen = ideal_record_length(dp->name);

    int newRecLen;
    if(dp->rec_len - idealLen >= needLen)
    {
        // Space available in this block
        newRecLen = dp->rec_len - idealLen;
        dp->rec_len = idealLen;

        // Advance to the next entry
        cp += dp->rec_len;
        dp = (DIR *)cp;
        dp->inode = InodeNum;
        dp->name_len = strlen(name);
        strncpy(dp->name, name, dp->name_len);

        dp->rec_len = newRecLen;
    }
    else
    {
        // Block is full allocate a new block
        BlockNum = balloc(pip->dev);
        dp = (DIR *)buf;
        dp->inode = InodeNum;
        dp->name_len = strlen(name);
        strncpy(dp->name, name, dp->name_len);
        dp->rec_len = BLOCK_SIZE;               // Rec takes entire block
        addLastBlock(pip, BlockNum);
    }

    // Write the block into disk
    put_block(pip->dev, BlockNum, buf);

    // Mark parent minode dirty
    pip->dirty = 1;

    // Increase the link count
    pip->INODE.i_links_count++;

    bzero(buf, BLOCK_SIZE);
    searchForParentInode(pip->dev, pip->ino, &mip->INODE, buf);
    touch_update(buf);

    return 1;
}

int rm_dir(char *path)
{

}
//Mo: Prints the cwd using the running proc pointer
int pwd(char *pathstr)
{
    printf("INSIDE PWD()\n");
	char temp_name[128];
	char temp_path[256];
	char path[256];
	int ino_num, parent_ino_num;
	MINODE *LocalMinoPrntPtr, *LocalMinoPtr;

	strcpy(path, "");

	LocalMinoPtr = running->cwd;

	findinode(LocalMinoPtr, &ino_num, &parent_ino_num);


	while(parent_ino_num != ino_num)
	{
		// test
        printf("PINoN = %d, LinoN = %d\n", parent_ino_num, ino_num);

		LocalMinoPrntPtr = (MINODE *)iget(dev, parent_ino_num);

		// test
        printf("before get ino name\n");
		getinoname(LocalMinoPrntPtr, ino_num, temp_name);


		// test
		printf("outside get inode name function\n");

		strcpy(temp_path, path);
        strcpy(path, temp_name);
        strcat(path, "/");
        strcat(path, temp_path);
		findinode(LocalMinoPrntPtr, &ino_num, &parent_ino_num);
	}
	strcpy(temp_path, path);
	strcpy(path, "/");
    strcat(path, temp_path);

    printf("%s\n", path);
    return 0;
}

int creat_file(char *path)
{

}
int rm_file(char *path)
{

}
int my_chmod(char *path)
{

}
int touch_update(char *path)
{

}

/* Exits the program and writes all dirty minode back disk */
int quit(char *path)
{
    int i;
    char strtemp[256];

    // Step through every fd[i] of the running procs
    for(i = 0; i < NFD; i++)
    {
        if(running->fd[i]) {

            snprintf(strtemp, 10, "%d", i);         //copys the name of the file currently open
            // Close the file
            close(strtemp);
        }
    }

    MINODE *mip;
    // Travers the MemoryMinodeTable
    for(i = 0; i < NMINODES; i++)
    {
        mip = &MemoryInodeTable[i];
        if(mip->refCount > 0)
        {
            // Check if the minode has been modified by check its dirty bit
            if(mip->dirty)
            {
                mip->refCount = 1;
                iput(mip->dev, mip);            // write memory inode back disk
            }
        }
    }

    printf("Thank you for trying out cs360ext2fs. Goodby!\n");
    exit(EXIT_SUCCESS);
}


// Helper Functions
int ls_dir(MINODE *mip)
{
    INODE *ip = &mip->INODE;         // Saves typing
    MINODE *cip;                     // child inode pointer

    int mdev = mip->dev;

    DIR *dp;
    char buf[BLOCK_SIZE], *cp;

    int i_blks = ip->i_size / BLOCK_SIZE;
    printf("ls directory: dev = %d blksize = %d\n", dev, i_blks);

    int i = 0;

    // Search i_blocks of INODE 0 - 11.. We dont need indirect and double indirect blocks
    while(i < 12)
    {
        if(ip->i_block[i] == 0 || i >= 12)
            break;

        get_block(mdev, ip->i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;

        while(cp < buf + BLOCK_SIZE)
        {
            char *temp = strdup(dp->name);
            temp[dp->name_len] = 0;

            cip = iget(mdev, dp->inode);
            ls_file(cip, temp);

            //printf("ls done! \n");

            iput(mdev, cip);

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        i++;
    }

    return 0;

}

int ls_file(MINODE *mip, char *fname)
{

    static const char *permissions = "rwxrwxrwx";
    char *str_time;

    if(mip == NULL)
    {
        printf("Error: Memory Inode is Null\n");
        return -1;
    }

    INODE *ip = &mip->INODE;

    // Get info from INODE Struct
    u16 mode   = ip->i_mode;
    u16 links  = ip->i_links_count;
    u16 uid_t  = ip->i_uid;
    u16 gid    = ip->i_gid;
    u16 size   = ip->i_size;

    str_time = ctime((time_t *)&ip->i_mtime);
    str_time[strlen(time) - 1] = 0;

    switch(mode & 0xF000)
    {
        case 0x8000: putchar('-');      // regular file
            break;
        case 0x4000: putchar('d');      // directory
            break;
        case 0xA000: putchar('l');      // link
            break;
        default:    putchar('$');
    }


    int i = 0, len = strlen(permissions);
    while(i < len)
    {
        if(mode & (1 << len - 1 - i))
        {
            putchar(permissions[i]);
        }
        else
        {
            putchar('-');
        }

        i++;
    }

    // Display file inode information
    printf("%4u %4u %4u %4u %4u %s\n", mode, links, uid_t, gid, size, fname);

    return 1;


}

int addLastBlock(MINODE *pip, int bnumber)
{
    int buf[256];
    int buf2[256];
    int i, j, newBlk, newBlk2;
     //Find last used block in parents directory
    for(i = 0; i < 12; i++) //Check direct blocks
    {
        if(pip->INODE.i_block[i] == 0) {pip->INODE.i_block[i] = bnumber; return 1;}
    }
    if(pip->INODE.i_block[12] == 0) //Have to make indirect block
    {
        newBlk = balloc(pip->dev);
        pip->INODE.i_block[12] = newBlk;
        memset(buf, 0, 256);
        get_block(pip->dev, newBlk, (char*)buf);
        buf[0] = bnumber;
        put_block(pip->dev, newBlk, (char*)buf);
        return 1;
    }
    memset(buf, 0, 256);
    get_block(pip->dev, pip->INODE.i_block[12], (char*)buf);
    for(i = 0; i < 256; i++)
    {
            if(buf[i] == 0) {buf[i] = bnumber; return 1;}
    }
    if(pip->INODE.i_block[13] == 0) //Make double indirect block
    {
        newBlk = balloc(pip->dev);
        pip->INODE.i_block[13] = newBlk;
        memset(buf, 0, 256);
        get_block(pip->dev, newBlk, (char*)buf);
        newBlk2 = balloc(pip->dev);
        buf[0] = newBlk2;
        put_block(pip->dev, newBlk, (char*)buf);
        memset(buf2, 0, 256);
        get_block(pip->dev, newBlk2, (char*)buf2);
        buf2[0] = bnumber;
        put_block(pip->dev, newBlk2, (char*)buf2);
        return 1;
    }
    memset(buf, 0, 256);
    get_block(pip->dev, pip->INODE.i_block[13], (char*)buf);
    for(i = 0; i < 256; i++)
    {
            if(buf[i] == 0)
            {
                newBlk2 = balloc(pip->dev);
                buf[i] = newBlk2;
                put_block(pip->dev, pip->INODE.i_block[13], (char*)buf);
                memset(buf2, 0, 256);
                get_block(pip->dev, newBlk2, (char*)buf2);
                buf2[0] = bnumber;
                put_block(pip->dev, newBlk2, (char*)buf2);
                return 1;
            }
            memset(buf2, 0, 256);
            get_block(pip->dev, buf[i], (char*)buf2);
            for(j = 0; j < 256; j++)
            {
                if(buf2[j] == 0) {buf2[j] = bnumber; return 1;}
            }
    }
    printf("ERROR: COULD NOT ADD BLOCK TO INODE\n");
    return -1;
}
