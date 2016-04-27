#include "ext2_utils.h"


MINODE *iget(int mdev, int inumber)
{
    char buf[BLOCK_SIZE];
    MINODE *mip;
    INODE *ip;

    int blk, offset;

    // Step 1: Search through the MINODE MemoryInodeTable[] array
    int i;
    for(i = 0; i < NMINODES; i++)
    {
        mip = &MemoryInodeTable[i];

        // Case 1: Check if the inumber already exists inside the array
        if(mip->refCount > 0 && mip->ino == inumber)
        {
            mip->refCount++;            // increase the The reference Count if the inumber exists inside the  MemoryInodeTable[i];
            return mip;
        }
    }


        // Case 2: inumber doesnt exists inside  MemoryInodeTable[i];
        //         We must find an empty location iside the MemoryInodeTable[i];

    i = 0;
    while(MemoryInodeTable[i].refCount > 0 && i < NMINODES)
    {
        i++;
    }

        // We must check for out of bounds index
    if(i >= NMINODES){ printf("Error: MemoryInodeTable is full\n\n"); return 0; }
    //printf("Free MemoryInodeTable found at index i = %d\n", i++);

    // Step 2: get block_number and offset using mailman's algorithm
    // Remember inodes start with 1 so we need to subtract 1
    blk     = (inumber - 1) / 8 + inodeBegin;
    offset  = (inumber - 1) % 8;

    // Step 3: read this blk into buffer
    bzero(buf, BLOCK_SIZE);
    get_block(mdev, blk, buf);

    // Inode is found at buf[offset] location the offset in this case is the house number where the inode lives
    ip = (INODE *)buf + offset;

    // Step 4: Copy this inode from disk into the MemoryInodeTable array
    // remember: i = index of free location to insert INODE inside MemoryInodeTable Array
    mip = &MemoryInodeTable[i];
    memcpy(&mip->INODE, ip, sizeof(INODE));

    // Step 5: set the Inode's dev = mdv ino= number and refCount = 1 and dirty = 0 mounted = 0 mountPtr = NULL
    mip->dev = mdev;
    mip->ino = inumber;
    mip->refCount = 1;
    mip->dirty = 0;
    mip->mounted = 0;
    mip->mountptr = NULL;


    // Finally return the the MemoryMinodeTable[i]
    return mip;

}

int iput(int mdev, MINODE *mip)
{
    char buf[BLOCK_SIZE];
    int blk, offset;
    INODE *ip;

   // printf("iput(): dev = %d\n", mdev);
    //printf("iput() decrementing inode refCount = %d\n", mip->refCount);

    // Step 1: decrease the inode refCount by 1
    mip->refCount--;
    if(mip->refCount > 0)
    {
        //printf("iput() refCount = %d\n", mip->refCount);
        return 0;
    }

    // Step 2: check if the Inode has not been modified
    if(mip->dirty == 0)
    {
        //printf("iput() inode has not been modified : dirty = %d\n", mip->dirty);
        return 0;           // No need to put Inode back into device since it has not been modified
    }

    // Step 3: Write Inode back into disk using MailMan's Algorith to find blk and offset
    blk     = (mip->ino - 1) / inodeBegin;
    offset  = (mip->ino -1) % 8;
    //printf("Preparing to write inode to disk @ blk = %d offset = %d\n", blk, offset);

    // Step 4: read the block into the buffer using the blk number
    get_block(mip->dev, blk, buf);
    ip = (INODE *)buf;

    // Step 5: Copy the In_Memory INODE struct into ip (INODE inside buffer array)
    memcpy(ip, &(mip->INODE), sizeof(INODE));

    // Step 6: Write the block back into disk
    put_block(mip->dev, blk, buf);

    return 0;

}
int put_block(int mdev, int blk, char *buf)
{
    lseek(mdev, (long)(blk * BLOCK_SIZE), SEEK_SET);
    write(mdev, buf, BLOCK_SIZE);

    return 0;
}

void get_block(int mdev, int blk, char *buf)
{
    lseek(mdev, blk * BLOCK_SIZE, SEEK_SET);
    read(mdev, buf, BLOCK_SIZE);
}


int get_super_block(int dev, char *buf)
{
    get_block(dev, SUPERBLOCK, buf);
}
int get_gd_block(int dev, char *buf)
{
    get_block(dev, GDBLOCK, buf);
}


int check_ext2(SUPER *sp)
{
    if(sp->s_magic != SUPER_MAGIC)
        return -1;

    return 1;
}


u32 getino(int mdev, char *path)
{

    char buf[BLOCK_SIZE];
    u32 inumber;


    char **tokens = NULL;
    char *pcopy = strdup(path);
    // Step 1: Path exists. Get the tokens from path
    //         Path doesnt exists then inumber = running->cwd->ino;
    if(path && path[0])
    {
        tokens = tokenize(pcopy);
    }
    else
    {
        inumber = running->cwd->ino;
        return inumber;
    }


    INODE  *ip;
    MINODE *mip;
    // Step 2: check if path starts with / then path is absolute
    //         start from ip = root->INODE, inumber = root->ino;
    //         else: path is relative to current directory ino
    if(path[0] == '/')
    {
        ip = &(root->INODE);        // start from root Inode
        inumber = root->ino;        // inumber is root->ino
    }
    else
    {
       ip = &(running->cwd->INODE); // path relative to current working directory.
    }

    int i = 0;

    // Step 3: Go through each individual token in tokens[][] and find its inumber
    while(tokens[i] != NULL)
    {

        printf("getino: searching tokens[i] = %s\n", tokens[i]);
        // Search the Datablocks of ip->i_block for inumber using the dev tokens[i] and ip (INODE where we begin searching)
        inumber = search(mdev, tokens[i], ip);
        if(inumber <= 0)
        {
            printf("getino: error invalid path\n");
            if(mip)
            {
                // Put the MemoryInodeTable back into device
                iput(mip->dev, mip);
                return -1;
            }
        }

        if(mip)
        {
            // Put the MemoryInodeTable back into device
            iput(mip->dev, mip);
        }

        i++;              // advance to the next token
        if(tokens[i])
        {
            // Get MemoryInodeTable mip from device
            mip = iget(mdev, inumber);
            ip  = &mip->INODE;
        }
    }

    i =0;
    // free up the dymaically allocated tokens
    while(tokens[i])
    {
        free(tokens[i++]);
    }

    // Write memory inode back to disk
    iput(mip->dev, mip);

    return inumber;

}

u32 search(int mdev, char *name, INODE *ip)
{
    char buf[BLOCK_SIZE];
    char dnamestr[256], *cp;
    DIR  *dp;

    // Step 1: Search the datablocks of ip. Assume only direct data blocks
    int i;
    for(i = 0; i < 12; i++)
    {
        // Step 2: if i_block[i] is 0 then break
        if(ip->i_block[i] == 0)
            break;

        // Read in the i_block[i] into the buffer
        int blk = ip->i_block[i];
        get_block(mdev, blk, buf);

        cp = buf;
        dp = (DIR *)buf;

        // Step 3: step through all the entries to find a matching dir name
        while(cp < buf + BLOCK_SIZE)
        {

            memset(dnamestr,0, 256);
            // Copy the dp->name into tempname
            strncpy(dnamestr, dp->name, dp->name_len);
            printf("search: dp->name = %s\n", dnamestr);
            if(strcmp(dnamestr, name) == 0)
            {
                return dp->inode;
            }
            // To go to the next entry we must add the rec_len to cp
            cp += dp->rec_len;
            dp = (DIR *)cp;             // advance dp to point to the next record
        }

        printf("search: endwhile\n");
    }

    // if it gets to here then return 0 because entry is not found
    return 0;
}

/* mip (memory inode ptr) is the inode data member (minode) we are looking into
 * and myinode is the current inode number's save location and
 * parentino is parent inode numbers's save location.
 * This function takes in mip and gets it's and it's parents inode
 * number.
*/
u32 findinode(MINODE *mip, int *myinode, int *parentino)
{
	char *cp;
	char buf[BLOCK_SIZE];
	DIR *localdirinoptr;
	INODE *localinoptr;

	// test
	printf("Inside find inode function\n");

	localinoptr = &mip->INODE;
	get_block(mip->dev, localinoptr->i_block[0], buf);
	cp = buf;

	// set myinode pointer
	localdirinoptr = (DIR *)buf;
	*myinode = localdirinoptr->inode;

	// set myinodes parents pointer
	cp += localdirinoptr->rec_len; // Increment cp to next address in buff
	localdirinoptr = (DIR *)cp;
	*parentino = localdirinoptr->inode;

	return 0;
}


/* Gets the local inode's string name using the the parent inode pointer
  to look through the parents inode direct blocks until found then
  return the name through perameter pointer. Return 1 if found else 0
 */
u32 getinoname(MINODE *parent, int myinode, char *inoname)
{
	int i = 0;
	int found = 0;
	char tempname[128];
	char buf[BLOCK_SIZE];
	char *cp;
	DIR *localdirinoptr;
	INODE *localinoptr;
	//
	localinoptr = &parent->INODE;

	for(i = 0; found == 0 && i < 12; i++)
	{
		get_block(dev, localinoptr->i_block[i], buf);
		cp = buf;
		localdirinoptr = (DIR *)buf;
		strcpy(tempname, localdirinoptr->name);

		// ensure tempname ends with null terminator
		tempname[localdirinoptr->name_len] = '\0';

		// test
		printf("Inside get inode name function\n");


		while(cp < (buf + BLOCK_SIZE))
		{
			// Right Dir entery found!
			if(localdirinoptr->inode == myinode)
			{
				strcpy(inoname, tempname);
				found = 1;
				break;
			}

			// Increment cp to next address in buff
			cp += localdirinoptr->rec_len;
			localdirinoptr = (DIR *)cp;
			strcpy(tempname, localdirinoptr->name);
			// ensure tempname ends with null terminator
			tempname[localdirinoptr->name_len] = '\0';

		}

	}

	return found;
}


/***************** Bit Functions **************/
/* The i in the perameters is the bit, buf is the buffer we are searching into.
 * We use mailmans algorithm. Note 1 bit reps a block of size 4kB*/
int tst_bit(char *buf, int i)
{

    printf("tst_bit()\n");
    int byte, offset;
    // Step 1: Use mailman's  algorithm
    byte = i / 8;
    offset = i % 8;

    // bit is set to 0 or 1
    int bit = (*(buf + byte) >> offset) & 1;

    //printf("tst_bit(): bit = %d\n", bit);

    return bit;
}
int set_bit(char *buf, int i)
{
    int byte, offset;
	byte = i/8;
	offset = i%8;
	buf[byte] |= (1 << offset);
	return 0;
}
// Sets bit to 0 to say inode/block is free
int unset_bit(char *buf, int i)
{
	int byte, offset;
	byte = i/8;
	offset = i%8;
	buf[byte] &= (0 << offset);
	return 0;
}

/***************** Inode,Block Alloc/Dealloc Functions **************/

// Mo: This function decriments the free_inodes_count members of
// super and group discriptor blocks by 1
void dec_free_inodes(int mdev)
{

	char buf[BLOCK_SIZE];


	printf("Decrimented free inodes count from SUPER and GD by 1\n");
	// Decriment from Super structure.
	get_super_block(mdev, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count--;
	put_block(mdev, SUPERBLOCK, buf);

	// Decriment from Group Discriptor structure.
	get_gd_block(mdev, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	put_block(mdev, GDBLOCK, buf);
}
// Mo: This function incriments the free_inodes_count members of
// super and group discriptor blocks by 1
void inc_free_inodes(int mdev)
{
	char buf[BLOCK_SIZE];


	printf("Incrimented free inodes count from SUPER and GD by 1\n");

    get_super_block(mdev, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count++;
	put_block(mdev, SUPERBLOCK, buf);

	// Incriment from Group Discriptor structure.
	get_gd_block(mdev, buf);
	gp = (GD *)buf;

	gp->bg_free_inodes_count++;
	put_block(dev, GDBLOCK, buf);
}
// Mo: This function decriments the free_blocks_count members of
// super and group discriptor blocks by 1
void dec_free_blocks(int mdev)
{
	SUPER *sp;
	GD *gp;
	char buf[BLOCK_SIZE];

	// Decriment from Super structure.
	get_block(dev, SUPERBLOCK, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count--;
	put_block(mdev, SUPERBLOCK, buf);

	// Decriment from Group Discriptor structure.
	get_block(mdev, GDBLOCK, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count--;
	put_block(mdev, GDBLOCK, buf);

	printf("Decrimented free blocks count from SUPER and GD by 1\n");
}
// Mo: This function incriments the free_blocks_count members of
// super and group discriptor blocks by 1
void inc_free_blocks(int mdev)
{
	char buf[BLOCK_SIZE];

	// Incriment from Super structure.
	get_super_block(mdev, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count++;
	put_block(mdev, SUPERBLOCK, buf);

	// Incriment from Group Discriptor structure.
	get_gd_block(mdev, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count++;
	put_block(mdev, GDBLOCK, buf);

	printf("Incrimented free blocks count from SUPER and GD by 1\n");
}



//Mo: Utility function to allocate an Inode
// & returns inode number if the're free inodes
int ialloc(int mdev)
{
    printf("Allocating inode in dev %d\n", mdev);
    char buf[BLOCK_SIZE];

    printf("reading imap block #%d from disk\n", imap);
    // Step 1: get the imap block from disk into the buf
    get_block(mdev, imap, buf);
    printf("ninodes = %d\n", ninodes);

    // ninodes is global number of inodes in disk.
    int i = 0;
    while(i < ninodes)
    {
        // Find a free inode bitmap
        if(tst_bit(buf, i) == 0)
        {
            printf("free bit found: i = %d\n", i);

            // Step 2: set the bit
            set_bit(buf, i);
           // Step 3: Write inode changes back into fs structure
			put_block(mdev, imap, buf);

			 // Step 4: decriment # of free inodes since we're alloc
			dec_free_inodes(mdev);

			// Step 5:Return inode number location
			return i+1;
        }

        i++;
    }
	printf("no more free inodes in ialloc()\n");
	return 0;
}
//Mo: Utility function to allocate a block number
// given the file discriptor mdev & returns block number
int balloc(int mdev)
{
	char buf[BLOCK_SIZE];
	printf("Allocating block in dev %d\n", mdev);
	printf("reading block bit map #%d from disk\n", bmap);
    // Step 1: get the bmap block from disk into the buf
    get_block(mdev, bmap, buf);

	// nblocks is global number of blocks in disk.
    int i = 0;
    while(i < nblocks)
    {

        // Find a free block from bitmap
        if(tst_bit(buf, i) == 0)
        {
            printf("free bit found: i = %d\n", i);

            // Step 2: set the bit
            set_bit(buf, i);
            // Step 3: decriment # of free blocks since we're alloc
		    dec_free_blocks(mdev);

		    // Write 0's into buffer
		    bzero(buf, BLOCK_SIZE);

			// Step 4: Write block changes back into fs structure
			put_block(mdev, bmap, buf);

			// Step 5:Return block number location
			return i+1;
        }

        i++;
    }
	printf("no more free blocks in balloc()\n");

    return 0;
}
// deallocates inode given a inode number, ino
int idealloc(int mdev, int ino)
{
	char buf[BLOCK_SIZE];

	// Read inode bitmap to buffer
	get_block(mdev, imap, buf);
	// set bit in ino location in buffer to 0 since dealloc
	unset_bit(buf, ino);

	// Write inode changes back into fs structure
	put_block(mdev, imap, buf);

    // decriment # of free inodes cause dealloc
	inc_free_inodes(mdev);
	return 0;
}
// deallocates block given a block number, bno
int bdealloc(int mdev, int bno)
{
	char buf[BLOCK_SIZE];

	// Read block bitmap to buffer
	get_block(mdev, bmap, buf);
	// set bit in bno location in buffer to 0 since dealloc
	unset_bit(buf, bno);
	// Write block changes back into fs structure
	put_block(mdev, bmap, buf);

	// decriment # of free blocks cause dealloc
	inc_free_blocks(mdev);
	return 0;
}

int ideal_record_length(const int name_len)
{
    return (4 * ((8 + name_len + 3) / 4));
}

int needed_length(const int name_len)
{
    // the needed length is
   return (4 * ((8 + name_len + 3) / 4));
}
u32 searchForParentInode(int mdev, int ino, INODE *ip, char *temp)
{

    char buf[BLKSIZE];
    char *cp;
    DIR *dp;

    int i;
    for(i = 0; i < 12; i++)
    {
        if(ip->i_block[i] == 0){ break;}
        get_block(mdev, ip->i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;

        while(cp < buf+BLKSIZE)
        {
            if(ino == dp->inode) //Found the right inode
            {
               strncpy(temp, dp->name, dp->name_len);
               return 1;
            }
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }
    }
    return 0;
}

int findLastBlock(MINODE *pip)
{
    int buf[256];
    int buf2[256];
    int bnumber, i, j;

     //Find last used block in parents directory
    if(pip->INODE.i_block[0] == 0) {return 0;}
    for(i = 0; i < 12; i++) //Check direct blocks
    {
        if(pip->INODE.i_block[i] == 0)
        {
            return (pip->INODE.i_block[i-1]);
        }
    }
    if(pip->INODE.i_block[12] == 0) {return pip->INODE.i_block[i-1];}
    get_block(dev, pip->INODE.i_block[12], (char*)buf);
    for(i = 0; i < 256; i++)
    {
            if(buf[i] == 0) {return buf[i-1];}
    }
    if(pip->INODE.i_block[13] == 0) {return buf[i-1];}
    //Print dirs in double indirect blocks
    memset(buf, 0, 256);
    get_block(pip->dev, pip->INODE.i_block[13], (char*)buf);
    for(i = 0; i < 256; i++)
    {
            if(buf[i] == 0) {return buf2[j-1];}
            if(buf[i])
            {
                    get_block(pip->dev, buf[i], (char*)buf2);
                    for(j = 0; j < 256; j++)
                    {
                        if(buf2[j] == 0) {return buf2[j-1];}
                    }
            }
    }
}
