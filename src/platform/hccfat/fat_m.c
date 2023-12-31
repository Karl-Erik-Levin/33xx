/****************************************************************************
 *
 *            Copyright (c) 2003-2008 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci ut 110
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/

#include "Platform\hccfat\fat.h"
#include "Platform\hccfat\common.h"

#if (!FN_CAPI_USED)

/****************************************************************************
 *
 * fm_getversion
 *
 * returns with the filesystem version string
 *
 * RETURNS
 *
 * string pointer with version number
 *
 ***************************************************************************/

char *fm_getversion(void)
{
	return fn_getversion();
}

/****************************************************************************
 *
 * fm_initvolume
 *
 * initiate a volume, this function has to be called 1st to set physical
 * driver function to a given volume
 *
 * INPUTS
 *
 * drvnumber - which drive need to be initialized
 * driver_init - driver init function
 * driver_param - parameter to pass to driver init
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_initvolume(int drvnumber,F_DRIVERINIT driver_init,unsigned long driver_param)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_initvolume(fm,drvnumber,driver_init,driver_param);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_createdriver
 *
 * Creating a driver, it calls driver init function
 *
 * INPUTS
 *
 * driver - where to store created driver
 * driver_init - driver init function
 * driver_param - parameter to pass to driver init
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_createdriver(F_DRIVER **driver,F_DRIVERINIT driver_init,unsigned long driver_param)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_createdriver(fm,driver,driver_init,driver_param);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_initvolumepartition
 *
 * initiate a volume, this function has to be called 1st to set physical
 * driver function to a given volume and a given partition
 *
 * INPUTS
 *
 * drvnumber - which drive need to be initialized
 * driver - driver to be used for this volume
 * partition - selected partition on the drive
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_initvolumepartition(int drvnumber,F_DRIVER *driver,int partition)
{
	int ret;
	F_MULTI *fm;
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_initvolumepartition(fm,drvnumber,driver,partition);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_createpartition
 *
 * It creates partition on a media
 *
 * INPUTS
 *
 * driver - driver structure
 * parnum - number of partition in par parameter
 * par - F_PARTITION structure, partition descriptor
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_createpartition(F_DRIVER *driver,int parnum, F_PARTITION *par)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_createpartition(fm,driver,parnum,par);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_getpartition
 *
 * getting partition info from drive
 *
 * INPUTS
 *
 * driver - driver structure
 * parnum - number of partition entry in par parameter
 * par - F_PARTITION structure where the result goes to
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_getpartition(F_DRIVER *driver,int parnum, F_PARTITION *par)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_getpartition(driver,parnum,par);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_checkvolume
 *
 * Deletes a previously initialized volume.
 *
 * INPUTS
 *
 * drvnumber - number of drive to check
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_checkvolume (int drvnumber)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_checkvolume(fm,drvnumber);
	
	_f_mutex_put(fm);
	return ret;
}

/****************************************************************************
 *
 * fm_delvolume
 *
 * Deletes a previously initialized volume.
 *
 * INPUTS
 *
 * drvnumber - number of drive to delete
 *
 * RETURNS
 *
 * errorcode
 *
 ***************************************************************************/

int fm_delvolume (int drvnumber)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_delvolume(fm,drvnumber);
	
	/* if delvolume called releasedriver to delete mutex then this fm->pmutex reference is reset */
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_releasedriver
 *
 * releasing a driver
 *
 * INPUTS
 *
 * driver - driver structure
 *
 * RETURNS
 *
 * errorcode
 *
 ***************************************************************************/

int fm_releasedriver (F_DRIVER *driver)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_releasedriver(fm,driver);
	
	/* putting back the mutex is not necessary because the mutex is deleted in releasedriver,delvolume */
	
	return ret;
}


/****************************************************************************
 *
 * fm_get_volume_count
 *
 * Returns the number of mounted volumes
 *
 * RETURNS
 *
 * number of mounted volumes
 * or -1 if any error
 *
 ***************************************************************************/

int fm_get_volume_count (void)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return -1; /* busy! */
	}
	
	ret=fn_get_volume_count(fm);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_get_volume_list
 *
 * get active volume list. It puts into the given buffer the active volumes
 * number
 *
 * INPUTS
 *
 * buf - where function puts active volumes number,
 *		 buf size must be at least F_MAXVOLUME	integers
 *
 * RETURNS
 *
 * number of mounted volumes
 * or -1 if any error
 *
 ***************************************************************************/

int fm_get_volume_list (int *buf)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return -1; /* busy! */
	}
	
	ret=fn_get_volume_list(fm,buf);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_format
 *
 * format a media, 1st it checks existing formatting, then master boot record,
 * then media physical
 *
 * INPUTS
 *
 * drivenum - which drive format is needed
 * fattype - one of this definitions F_FAT12_MEDIA,F_FAT16_MEDIA,F_FAT32_MEDIA
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_format(int drivenum,long fattype)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_format(fm,drivenum,fattype);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_getcwd
 *
 * getting a current working directory of current drive
 *
 * INPUTS
 *
 * buffer - where to store current working folder
 * maxlen - buffer length (possible size is F_MAXPATH)
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_getcwd(char *buffer, int maxlen )
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_getcwd(fm,buffer,maxlen);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wgetcwd(wchar *buffer, int maxlen )
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wgetcwd(fm,buffer,maxlen);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_getdcwd
 *
 * getting a drive current working directory
 *
 * INPUTS
 *
 * drivenum - drive number of which drive current folder needed
 * buffer - where to store current working folder
 * maxlen - buffer length (possible size is F_MAXPATH)
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_getdcwd(int drivenum, char *buffer, int maxlen )
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_getdcwd(fm,drivenum,buffer,maxlen);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wgetdcwd(int drivenum, wchar *buffer, int maxlen )
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wgetdcwd(fm,drivenum,buffer,maxlen);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_chdrive
 *
 * Change current drive
 *
 * INPUTS
 *
 * drivenum - new current drive number (0-A, 1-B, ...)
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_chdrive(int drivenum)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_chdrive(fm,drivenum);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_getdrive
 *
 * Get current drive number
 *
 * RETURNS
 *
 * with the current drive number (0-A, 1-B,...)
 *
 ***************************************************************************/

int fm_getdrive(void)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return -1; /* busy! */
	}
	if (fm->f_curdrive==-1) 
	{
		return -1;
	}
	
	ret=fn_getdrive(fm);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_getfreespace
 *
 * get total/free/used/bad diskspace
 *
 * INPUTS
 *
 * drivenum - which drive free space is requested (0-A, 1-B, 2-C)
 * pspace - pointer where to store the information
 *
 * RETURNS
 * error code
 *
 ***************************************************************************/

int fm_getfreespace(int drivenum,FN_SPACE *pspace)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND;
	
	ret=fn_getfreespace(fm,drivenum,pspace);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_chdir
 *
 * change current working directory
 *
 * INPUTS
 *
 * dirname - new working directory name
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fm_chdir(const char *dirname)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_chdir(fm,dirname);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wchdir(const wchar *dirname)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wchdir(fm,dirname);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif


/****************************************************************************
 *
 * fm_mkdir
 *
 * making a new directory
 *
 * INPUTS
 *
 * dirname - new directory name
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_mkdir(const char *dirname)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_mkdir(fm,dirname);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wmkdir(const wchar *dirname)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wmkdir(fm,dirname);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_rmdir
 *
 * Remove directory, only could be removed if empty
 *
 * INPUTS
 *
 * dirname - which directory needed to be removed
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_rmdir(const char *dirname)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_rmdir(fm,dirname);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wrmdir(const wchar *dirname)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wrmdir(fm,dirname);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_findfirst
 *
 * find a file(s) or directory(s) in directory
 *
 * INPUTS
 *
 * filename - filename (with or without wildcards)
 * find - where to store found file information
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_findfirst(const char *filename,FN_FIND *find)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_findfirst(fm,filename,find);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wfindfirst(const wchar *filename,FN_WFIND *find)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wfindfirst(fm,filename,find);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_findnext
 *
 * find further file(s) or directory(s) in directory
 *
 * INPUTS
 *
 * find - where to store found file information (findfirst should call 1st)
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_findnext(FN_FIND *find)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_findnext(fm,find);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wfindnext(FN_WFIND *find)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wfindnext(fm,find);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_rename
 *
 * Rename file or directory
 *
 * INPUTS
 *
 * filename - filename or directory name (with or without path)
 * newname - new name of the file or directory (without path)
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fm_rename(const char *filename, const char *newname)
{
	int ret;
	F_MULTI *fm;
	char ismove=0;
	int a;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	for (a=0;;a++)
	{
		char ch=newname[a];
		if (ch=='/' || ch=='\\')
		{
			ismove=1;
			break;
		}
		if (!ch) break;
	}
	
	if (ismove) ret=fn_move(fm,filename,newname);
	else ret=fn_rename(fm,filename,newname);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wrename(const wchar *filename, const wchar *newname)
{
	int ret;
	F_MULTI *fm;
	char ismove=0;
	int a;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	for (a=0;;a++)
	{
		wchar ch=newname[a];
		if (ch=='/' || ch=='\\')
		{
			ismove=1;
			break;
		}
		if (!ch) break;
	}
	
	if (ismove) ret=fn_wmove(fm,filename,newname);
	else ret=fn_wrename(fm,filename,newname);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_move
 *
 * move file or directory
 *
 * INPUTS
 *
 * filename - filename or directory name (with or without path)
 * newname - new name of the file or directory (with path)
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fm_move(const char *filename, const char *newname)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_move(fm,filename,newname);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wmove(const wchar *filename, const wchar *newname)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wmove(fm,filename,newname);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_filelength
 *
 * Get a file length
 *
 * INPUTS
 *
 * filename - file whose length is needed
 *
 * RETURNS
 *
 * length of the file or -1 if any error
 *
 ***************************************************************************/

long fm_filelength(const char *filename)
{
	long ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return -1; /* busy! */
	}
	
	ret=fn_filelength(fm,filename);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
long fm_wfilelength(const wchar *filename)
{
	long ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return -1; /* busy! */
	}
	
	ret=fn_wfilelength(fm,filename);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_close
 *
 * close a previously opened file
 *
 * INPUTS
 *
 * filehandle - which file needs to be closed
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_close(FN_FILE *filehandle)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_close(fm,filehandle);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_flush
 *
 * flushing current content a file into physical.
 *
 * INPUTS
 *
 * filehandle - which file needs to be flushed
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fm_flush(FN_FILE *filehandle)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_flush(fm,filehandle);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_open
 *
 * open a file for reading/writing/appending
 *
 * INPUTS
 *
 * filename - which file need to be opened
 * mode - string how to open ("r"-read, "w"-write, "w+"-overwrite, "a"-append
 *
 * RETURNS
 *
 * FN_FILE pointer if successfully
 * 0 - if any error
 *
 ***************************************************************************/

FN_FILE *fm_open(const char *filename,const char *mode)
{
	FN_FILE *ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return 0; /* busy! */
	}
	
	ret=fn_open(fm,filename,mode);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
FN_FILE *fm_wopen(const wchar *filename,const wchar *mode)
{
	FN_FILE *ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return 0; /* busy! */
	}
	
	ret=fn_wopen(fm,filename,mode);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

#ifdef FILEOPENOPT
/****************************************************************************
 * DATATON PICKUP - Alternative file open functions
 ***************************************************************************/

/****************************************************************************
 *
 * fm_openopt
 *
 * open a file for reading by it's startcluster
 *
 * INPUTS
 *
 * startcluster - where the file starts
 * filesize - size of file (in bytes)
 *
 * RETURNS
 *
 * FN_FILE pointer if successfully
 * 0 - if any error
 *
 ***************************************************************************/
FN_FILE *fm_openopt(const unsigned long startcluster,const unsigned long filesize)
{
	FN_FILE *ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return 0; /* busy! */
	}
	
	ret=fn_openopt(fm,startcluster,filesize);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_getfilesize
 *
 * Returns an open file size
 *
 * INPUTS
 *
 * filehandle - file
 *
 * RETURNS
 *
 * Size in bytes
 *
 ***************************************************************************/
unsigned long fm_getfilesize(FN_FILE *filehandle)
{
	F_MULTI *fm;
	unsigned long ret;
	
	/* Check if file is open */
	if(!filehandle)
		return 0;
	
	if (fnGetTask(&fm)) 
	{
		return 0; /* busy! */
	}
	
	ret = fn_getfilesize(fm, filehandle);
	
	_f_mutex_put(fm);
	
	/* Return file size */
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_read
 *
 * read data from file
 *
 * INPUTS
 *
 * buf - where the store data
 * size - size of items to be read
 * size_st - number of items need to be read
 * filehandle - file where to read from
 *
 * RETURNS
 *
 * with the number of read bytes
 *
 ***************************************************************************/

long fm_read(void *buf,long size,long size_st,FN_FILE *filehandle)
{
	long ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return 0; /* busy! */
	}
	
	ret=fn_read(fm,buf,size,size_st,filehandle);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_write
 *
 * write data into file
 *
 * INPUTS
 *
 * buf - where the store data
 * size - size of items to be read
 * size_st - number of items need to be read
 * filehandle - file where to read from
 *
 * RETURNS
 *
 * with the number of written bytes
 *
 ***************************************************************************/

long fm_write(const void *buf,long size,long size_st,FN_FILE *filehandle)
{
	long ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return 0; /* busy! */
	}
	
	ret=fn_write(fm,buf,size,size_st,filehandle);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_seek
 *
 * moves position into given offset in given file
 *
 * INPUTS
 *
 * filehandle - FN_FILE structure which file position needed to be modified
 * offset - relative position
 * whence - where to calculate position (F_SEEK_SET,F_SEEK_CUR,F_SEEK_END)
 *
 * RETURNS
 *
 * 0 - if successfully
 * other - if any error
 *
 ***************************************************************************/

int fm_seek(FN_FILE *filehandle,long offset,long whence)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_seek(fm,filehandle,offset,whence);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_tell
 *
 * Tells the current position of opened file
 *
 * INPUTS
 *
 * filehandle - which file needs the position
 *
 * RETURNS
 *
 * position in the file from start or -1 if any error
 *
 ***************************************************************************/

long fm_tell(FN_FILE *filehandle)
{
	long ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_tell(fm,filehandle);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_getc
 *
 * get a character from file
 *
 * INPUTS
 *
 * filehandle - file where to read from
 *
 * RETURNS
 *
 * with the read character or -1 if read was not successfully
 *
 ***************************************************************************/

int fm_getc(FN_FILE *filehandle)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return -1; /* busy! */
	}
	
	ret=fn_getc(fm,filehandle);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_putc
 *
 * write a character into file
 *
 * INPUTS
 *
 * ch - what to write into file
 * filehandle - file where to write
 *
 * RETURNS
 *
 * with the written character or -1 if any error
 *
 ***************************************************************************/

int fm_putc(int ch,FN_FILE *filehandle)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return -1; /* busy! */
	}
	
	ret=fn_putc(fm,ch,filehandle);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_rewind
 *
 * set the fileposition in the opened file to the begining
 *
 * INPUTS
 *
 * filehandle - which file needs to be rewinded
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_rewind(FN_FILE *filehandle)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_rewind(fm,filehandle);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_eof
 *
 * Tells if the current position is end of file or not
 *
 * INPUTS
 *
 * filehandle - which file needs the checking
 *
 * RETURNS
 *
 * 0 - if not EOF
 * other - if EOF or invalid file handle
 *
 ***************************************************************************/

int fm_eof(FN_FILE *filehandle)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_eof(fm,filehandle);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_gettimedate
 *
 * get a file time and date
 *
 * INPUTS
 *
 * filename - which file time and date wanted to be retrive
 * pctime - ctime of the file where to store
 * pcdate - cdate of the file where to store
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_gettimedate(const char *filename,unsigned short *pctime,unsigned short *pcdate)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_gettimedate(fm,filename,pctime,pcdate);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wgettimedate(const wchar *filename,unsigned short *pctime,unsigned short *pcdate)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wgettimedate(fm,filename,pctime,pcdate);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_stat
 *
 * get status information on a file
 *
 * INPUTS
 *
 * filename - which file time and date wanted to be retrive
 * stat - pointer where to store information
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_stat(const char *filename,F_STAT *stat)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_stat(fm,filename,stat);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wstat(const wchar *filename,F_STAT *stat)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wstat(fm,filename,stat);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_settimedate
 *
 * set a file time and date
 *
 * INPUTS
 *
 * filename - which file time and date wanted to be set
 * ctime - new ctime of the file
 * cdate - new cdate of the file
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_settimedate(const char *filename,unsigned short ctime,unsigned short cdate)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_settimedate(fm,filename,ctime,cdate);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wsettimedate(const wchar *filename,unsigned short ctime,unsigned short cdate)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wsettimedate(fm,filename,ctime,cdate);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_delete
 *
 * delete a file
 *
 * INPUTS
 *
 * filename - file which wanted to be deleted (with or without path)
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_delete(const char *filename)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_delete(fm,filename);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wdelete(const wchar *filename)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wdelete(fm,filename);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_getattr
 *
 * get file attribute
 *
 * INPUTS
 *
 * filename - which file attribute is needed
 * attr - pointer to a characterter where to store attribute information
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_getattr(const char *filename,unsigned char *attr)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_getattr(fm,filename,attr);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wgetattr(const wchar *filename,unsigned char *attr)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wgetattr(fm,filename,attr);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_setattr
 *
 * set attribute of a file
 *
 * INPUTS
 *
 * filename - which file attribute set
 * attr - new attribute of the file
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_setattr(const char *filename,unsigned char attr)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_setattr(fm,filename,attr);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
int fm_wsetattr(const wchar *filename,unsigned char attr)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_wsetattr(fm,filename,attr);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_getlabel
 *
 * get a label of a media
 *
 * INPUTS
 *
 * drivenum - drive number which label's is needed
 * label - char pointer where to store label
 * len - length of label buffer
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_getlabel(int drivenum, char *label, long len)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_getlabel(fm,drivenum,label,len);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_setlabel
 *
 * set a label of a media
 *
 * INPUTS
 *
 * drivenum - drive number which label's need to be set
 * label - new label for the media
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_setlabel(int drivenum, const char *label)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_setlabel(fm,drivenum,label);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fn_truncate
 *
 * truncate a file to a specified length, filepointer will be set to the
 * end
 *
 * INPUTS
 *
 * filename - which file need to be truncated
 * length - length of new file
 *
 * RETURNS
 *
 * filepointer or zero if fails
 *
 ***************************************************************************/

FN_FILE *fm_truncate(const char *filename,unsigned long length)
{
	FN_FILE *ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return 0; /* busy! */
	}
	
	ret=fn_truncate(fm,filename,length);
	
	_f_mutex_put(fm);
	
	return ret;
}

#ifdef HCC_UNICODE
FN_FILE *fm_wtruncate(const wchar *filename,unsigned long length)
{
	FN_FILE *ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) 
	{
		return 0; /* busy! */
	}
	
	ret=fn_wtruncate(fm,filename,length);
	
	_f_mutex_put(fm);
	
	return ret;
}
#endif

/****************************************************************************
 *
 * fm_get_oem
 *
 * Get OEM name
 *
 * INPUTS
 *
 * drivenum - drivenumber
 * str - where to store information
 * maxlen - length of the buffer
 *
 * RETURN
 *
 * errorcode or zero if successful
 *
 ***************************************************************************/

int fm_get_oem (int drivenum, char *str, long maxlen)
{
	F_MULTI *fm;
	int ret;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_get_oem(fm,drivenum,str,maxlen);
	_f_mutex_put(fm);
	return ret;
}

/****************************************************************************
 *
 * fm_ftruncate
 *
 * truncate a file to a specified length, filepointer will be set to the
 * end
 *
 * INPUTS
 *
 * filehandle - which file need to be truncated
 * length - length of new file
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int	fm_ftruncate(FN_FILE *filehandle,unsigned long length)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_ftruncate(fm,filehandle,length);
	
	_f_mutex_put(fm);
	
	return ret;
}

/****************************************************************************
 *
 * fm_seteof
 *
 * set end of file at the current position
 *
 * INPUTS
 *
 * filehandle - file where to read from
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int fm_seteof(FN_FILE *filehandle)
{
	int ret;
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	ret=fn_seteof(fm,filehandle);
	
	_f_mutex_put(fm);
	
	return ret;
}


/****************************************************************************
 *
 * fm_getlasterror
 *
 * returns with the last set error
 *
 * RETURNS
 *
 * error code which was set as lasterror
 *
 ***************************************************************************/

int fm_getlasterror()
{
	F_MULTI *fm;
	
	if (fnGetTask(&fm)) return F_ERR_TASKNOTFOUND; /* busy! */
	
	return fm->lasterror;
}

/****************************************************************************
 *
 * end of fat_m.c
 *
 ***************************************************************************/

#endif /* !FN_CAPI_USED */
