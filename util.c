/*********** util.c file ****************/

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char   gpath[256];
extern char   *name[64];
extern int    n;

extern int    fd, dev;
extern int    nblocks, ninodes, bmap, imap, inode_start;
extern char   line[256], cmd[32], pathname[256];
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
	// tokenize pathname in GLOBAL gpath[]; pointer by name[i]; n tokens
	int i = 0;
	strcpy(gpath,pathname);
	char * token;
	token = strtok(gpath,"/");
	while(token != NULL)
	{
		printf("token[%d] -> %s\n",i,token);
		name[i] = (char *)malloc(strlen(token) * sizeof(char));
		strcpy(name[i],token);
		token = strtok(NULL,"/");
		i++;
	}
	n = i;

}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
	int i;
	MINODE *mip;
	char buf[BLKSIZE];
	int blk, disp;
	INODE *ip;

	for (i=0; i<NMINODE; i++){
		mip = &minode[i];

		if (mip->refCount && mip->dev == dev && mip->ino == ino){
			mip->refCount++;
			printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
			return mip;
		}
	}

	for (i=0; i<NMINODE; i++){
		mip = &minode[i];
		if (mip->refCount == 0){
			//printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
			mip->refCount = 1;
			mip->dev = dev;
			mip->ino = ino;

			// get INODE of ino to buf    
			blk  = (ino-1) / 8 + inode_start;
			disp = (ino-1) % 8;

			//printf("iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);

			get_block(dev, blk, buf);
			ip = (INODE *)buf + disp;
			// copy INODE to mp->INODE
			mip->INODE = *ip;

			return mip;
		}
	}   
	printf("PANIC: no more free minodes\n");
	return 0;
}

int iput(MINODE *mip)
{
	int i, block, offset;
	char buf[BLKSIZE];
	INODE *ip;

	if (mip==0) 
		return;

	mip->refCount--;

	if (mip->refCount > 0) return;
	if (!mip->dirty)       return;

	/* write back */
	printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino); 

	block =  ((mip->ino - 1) / 8) + inode_start;
	offset =  (mip->ino - 1) % 8;

	/* first get the block containing this inode */
	get_block(mip->dev, block, buf);

	ip = (INODE *)buf + offset;
	*ip = mip->INODE;

	put_block(mip->dev, block, buf);

} 

int search(MINODE *mip, char *name)
{
	// YOUR serach() fucntion as in LAB 6
	
	char sbuf[BLKSIZE], temp[256];
	DIR *dp;
	char *cp;
	int i;

	for (i=0; i < 12; i++){  // assume DIR at most 12 direct blocks
		if (mip->INODE.i_block[i] == 0)
			break;
		printf("i_block number -> %d\n",mip->INODE.i_block[i]);
		// YOU SHOULD print i_block[i] number here
		get_block(fd, mip->INODE.i_block[i], sbuf);

		dp = (DIR*)sbuf;
		cp = sbuf;

		while(cp < sbuf + BLKSIZE){
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			/*printf("%4d %4d %4d %s\n", */
					/*dp->inode, dp->rec_len, dp->name_len, temp);*/
			if(!strcmp(name,temp))
			{
				printf("Found %s with inode %d\n",name,dp->inode);
				return dp->inode;
			}
			else
			{

			cp += dp->rec_len;
			dp = (DIR *)cp;
			}
		}
	}	
	return 0;
}

int getino(char *pathname)
{
	int i, ino, blk, disp;
	INODE *ip;
	MINODE *mip;

	printf("getino: pathname=%s\n", pathname);
	if (strcmp(pathname, "/")==0)
		return 2;

	if (pathname[0]=='/')
		mip = iget(dev, 2);
	else
		mip = iget(running->cwd->dev, running->cwd->ino);

	tokenize(pathname);

	for (i=0; i<n; i++){
		printf("===========================================\n");
		ino = search(mip, name[i]);

		if (ino==0){
			iput(mip);
			printf("name %s does not exist\n", name[i]);
			return 0;
		}
		iput(mip);
		mip = iget(dev, ino);
	}
	iput(mip);
	return ino;
}

