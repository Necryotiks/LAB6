/************* cd_ls_pwd.c file **************/

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char   gpath[256];
extern char   *name[64];
extern int    n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];
int ino;

#define OWNER  000700
#define GROUP  000070
#define OTHER  000007

char * t1 = "xwrxwrxwr-------";
char * t2 = "----------------";
void ls_file(int ino,char * name)
{
	char ftime[64];
	MINODE * mip = iget(dev,ino);
	INODE * ip = &(mip->INODE);
	printf("%4d ",ip->i_gid);
	printf("%4d ",ip->i_uid);
	printf("%8d ",(int)ip->i_size);
	strcpy(ftime,ctime(&ip->i_ctime));
	ftime[strlen(ftime)-1] = 0;
	printf("%s  ",ftime);
	printf("%s\n",basename(name));
	
}
void ls_dir(char * pathname)
{
	printf("pathname = %s\n",pathname);
	int i = 0;
	tokenize(pathname);
	if(n == 0)
	{
		char buf[1024],temp[256];
		char * cp;
		MINODE *mip = iget(dev, ino);
		get_block(dev,running->cwd->INODE.i_block[0],buf);
		DIR * dp = (DIR*)buf;
		cp = buf;
		while(cp < buf + 1024)
		{
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			printf("%4d %4d %4d %s\n",dp->inode, dp->rec_len, dp->name_len, temp);
			/*ls_file(dp->inode,temp);*/

			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	else
	{
		char buf[1024],temp[256];
		char * cp;
		ino = getino(pathname);
		if(ino == 0)
		{
			printf("Could not find %s\n",name[i]);
			exit(1);
		}
		MINODE *mip = iget(dev, ino);
		get_block(dev,mip->INODE.i_block[0],buf);
		DIR * dp = (DIR*)buf;
		cp = buf;
		while(cp < buf + 1024)
		{
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			printf("%4d %4d %4d %s\n",dp->inode, dp->rec_len, dp->name_len, temp);
			//ls_file segfaults
			/*ls_file(dp->inode,temp);*/

			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}



}
int change_dir()
{
	int i = 0;
	MINODE *mip;
	tokenize(pathname);
	if(n == 0)
	{
		iput(running->cwd);
		running->cwd = root;
		printf("cwd is now root.\n");
	}
	else
	{
		ino = getino(pathname);
		mip = iget(dev, ino);
		unsigned short int temp = mip->INODE.i_mode;
		temp = temp >> 12;
		temp = temp & 0XF;
		if((temp) == 0x4)
		{
			iput(running->cwd);
			running->cwd = mip;
		}
		else
		{
			printf("Change dir failure, not a directory.\n");
		}
	}
}
int list_file()
{
	ls_dir(pathname);
}

void rpwd(MINODE *wd)
{
	char myname[256];
	if(wd == root)
		return;
	else
	{
		char buf[1024],temp[256];
		char * cp;
		MINODE *mip = iget(dev, wd->INODE.i_block[0]);
		get_block(dev,mip->ino,buf);
		DIR * dp = (DIR*)buf;
		cp = buf;
		while(cp < buf + 1024)
		{
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			printf("%4d %4d %4d %s\n",dp->inode, dp->rec_len, dp->name_len, temp);
			if(!strcmp("..",temp))
			{
				break;
			}
			/*ls_file(dp->inode,temp);*/

			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		MINODE * pip = iget(dev,dp->inode);
//NOTE: does not print dir name
		rpwd(pip);
		printf("/%s",temp);
		/*printf("%d",wd->INODE.i_block[1]);*/

	}
	printf("\n");
}
int pwd(MINODE *wd)
{
	if(wd == root)
		printf("/");
	else
		rpwd(wd);

}



