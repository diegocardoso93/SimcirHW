#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "vfs_int.h"

#include "fatfs_prefix_lib.h"
#include "ff.h"
#include "fatfs_config.h"


static FRESULT last_result = FR_OK;

static const char* const volstr[_VOLUMES] = {_VOLUME_STRS};

static int is_current_drive = false;


// forward declarations
static int32_t myfatfs_close( const struct vfs_file *fd );
static int32_t myfatfs_read( const struct vfs_file *fd, void *ptr, size_t len );
static int32_t myfatfs_write( const struct vfs_file *fd, const void *ptr, size_t len );
static int32_t myfatfs_lseek( const struct vfs_file *fd, int32_t off, int whence );
static int32_t myfatfs_eof( const struct vfs_file *fd );
static int32_t myfatfs_tell( const struct vfs_file *fd );
static int32_t myfatfs_flush( const struct vfs_file *fd );
static uint32_t myfatfs_fsize( const struct vfs_file *fd );
static int32_t myfatfs_ferrno( const struct vfs_file *fd );

static int32_t  myfatfs_closedir( const struct vfs_dir *dd );
static vfs_item *myfatfs_readdir( const struct vfs_dir *dd );

static void        myfatfs_iclose( const struct vfs_item *di );
static uint32_t    myfatfs_isize( const struct vfs_item *di );
static int32_t    myfatfs_time( const struct vfs_item *di, struct vfs_time *tm );
static const char *myfatfs_name( const struct vfs_item *di );
static int32_t    myfatfs_is_dir( const struct vfs_item *di );
static int32_t    myfatfs_is_rdonly( const struct vfs_item *di );
static int32_t    myfatfs_is_hidden( const struct vfs_item *di );
static int32_t    myfatfs_is_sys( const struct vfs_item *di );
static int32_t    myfatfs_is_arch( const struct vfs_item *di );

static vfs_vol  *myfatfs_mount( const char *name, int num );
static vfs_file *myfatfs_open( const char *name, const char *mode );
static vfs_dir  *myfatfs_opendir( const char *name );
static vfs_item *myfatfs_stat( const char *name );
static int32_t  myfatfs_remove( const char *name );
static int32_t  myfatfs_rename( const char *oldname, const char *newname );
static int32_t  myfatfs_mkdir( const char *name );
static int32_t  myfatfs_fsinfo( uint32_t *total, uint32_t *used );
static int32_t  myfatfs_chdrive( const char *name );
static int32_t  myfatfs_chdir( const char *name );
static int32_t  myfatfs_errno( void );
static void      myfatfs_clearerr( void );

static int32_t myfatfs_umount( const struct vfs_vol *vol );


// ---------------------------------------------------------------------------
// function tables
//
static vfs_fs_fns myfatfs_fs_fns = {
  .mount    = myfatfs_mount,
  .open     = myfatfs_open,
  .opendir  = myfatfs_opendir,
  .stat     = myfatfs_stat,
  .remove   = myfatfs_remove,
  .rename   = myfatfs_rename,
  .mkdir    = myfatfs_mkdir,
  .fsinfo   = myfatfs_fsinfo,
  .fscfg    = NULL,
  .format   = NULL,
  .chdrive  = myfatfs_chdrive,
  .chdir    = myfatfs_chdir,
  .ferrno   = myfatfs_errno,
  .clearerr = myfatfs_clearerr
};

static vfs_file_fns myfatfs_file_fns = {
  .close     = myfatfs_close,
  .read      = myfatfs_read,
  .write     = myfatfs_write,
  .lseek     = myfatfs_lseek,
  .eof       = myfatfs_eof,
  .tell      = myfatfs_tell,
  .flush     = myfatfs_flush,
  .size      = myfatfs_fsize,
  .ferrno    = myfatfs_ferrno
};

static vfs_item_fns myfatfs_item_fns = {
  .close     = myfatfs_iclose,
  .size      = myfatfs_isize,
  .time      = myfatfs_time,
  .name      = myfatfs_name,
  .is_dir    = myfatfs_is_dir,
  .is_rdonly = myfatfs_is_rdonly,
  .is_hidden = myfatfs_is_hidden,
  .is_sys    = myfatfs_is_sys,
  .is_arch   = myfatfs_is_arch
};

static vfs_dir_fns myfatfs_dir_fns = {
  .close     = myfatfs_closedir,
  .readdir   = myfatfs_readdir
};

static vfs_vol_fns myfatfs_vol_fns = {
  .umount    = myfatfs_umount
};


// ---------------------------------------------------------------------------
// specific struct extensions
//
struct myvfs_vol {
  struct vfs_vol vfs_vol;
  char *ldrname;
  FATFS fs;
};

struct myvfs_file {
  struct vfs_file vfs_file;
  FIL fp;
};

struct myvfs_dir {
  struct vfs_dir vfs_dir;
  DIR dp;
};

struct myvfs_item {
  struct vfs_item vfs_item;
  FILINFO fno;
};


// ---------------------------------------------------------------------------
// exported helper functions for FatFS
//
inline void *ff_memalloc( UINT size )
{
  return malloc( size );
}

inline void ff_memfree( void *mblock )
{
  free( mblock );
}

// TODO
DWORD get_fattime( void )
{
  DWORD stamp;
  vfs_time tm;

  if (VFS_RES_OK == vfs_get_rtc( &tm )) {
    // sanity checks
    tm.year = (tm.year >= 1980) && (tm.year < 2108) ? tm.year : 1980;
    tm.mon  = (tm.mon  >= 1) && (tm.mon  <= 12) ? tm.mon  : 1;
    tm.day  = (tm.day  >= 1) && (tm.day  <= 31) ? tm.day  : 1;
    tm.hour = (tm.hour >= 0) && (tm.hour <= 23) ? tm.hour : 0;
    tm.min  = (tm.min  >= 0) && (tm.min  <= 59) ? tm.min  : 0;
    tm.sec  = (tm.sec  >= 0) && (tm.sec  <= 59) ? tm.sec  : 0;

    stamp = (tm.year-1980) << 25 | tm.mon << 21 | tm.day << 16 |
            tm.hour << 11 | tm.min << 5 | tm.sec;
  } else {
    // default time stamp derived from ffconf.h
    stamp = ((DWORD)(_NORTC_YEAR - 1980) << 25 | (DWORD)_NORTC_MON << 21 | (DWORD)_NORTC_MDAY << 16);
  }

  return stamp;
}


// ---------------------------------------------------------------------------
// volume functions
//
#define GET_FATFS_FS(descr) \
  const struct myvfs_vol *myvol = (const struct myvfs_vol *)descr; \
  FATFS *fs = (FATFS *)&(myvol->fs);

static int32_t myfatfs_umount( const struct vfs_vol *vol )
{
  GET_FATFS_FS(vol);
  (void)fs;

  last_result = f_mount( NULL, myvol->ldrname, 0 );

  free( myvol->ldrname );
  free( (void *)vol );

  return last_result == FR_OK ? VFS_RES_OK : VFS_RES_ERR;
}


// ---------------------------------------------------------------------------
// file functions
//
#define GET_FIL_FP(descr) \
  const struct myvfs_file *myfd = (const struct myvfs_file *)descr; \
  FIL *fp = (FIL *)&(myfd->fp);

static int32_t myfatfs_close( const struct vfs_file *fd )
{
  GET_FIL_FP(fd)

  last_result = f_close( fp );

  // free descriptor memory
  free( (void *)fd );

  return last_result == FR_OK ? VFS_RES_OK : VFS_RES_ERR;
}

static int32_t myfatfs_read( const struct vfs_file *fd, void *ptr, size_t len )
{
  GET_FIL_FP(fd);
  UINT act_read;

  last_result = f_read( fp, ptr, len, &act_read );

  return last_result == FR_OK ? act_read : VFS_RES_ERR;
}

static int32_t myfatfs_write( const struct vfs_file *fd, const void *ptr, size_t len )
{
  GET_FIL_FP(fd);
  UINT act_written;

  last_result = f_write( fp, ptr, len, &act_written );

  return last_result == FR_OK ? act_written : VFS_RES_ERR;
}

static int32_t myfatfs_lseek( const struct vfs_file *fd, int32_t off, int whence )
{
  GET_FIL_FP(fd);
  FSIZE_t new_pos;

  switch (whence) {
  default:
  case VFS_SEEK_SET:
    new_pos = off > 0 ? off : 0;
    break;
  case VFS_SEEK_CUR:
    new_pos = f_tell( fp );
    new_pos += off;
    break;
  case VFS_SEEK_END:
    new_pos = f_size( fp );
    new_pos += off < 0 ? off : 0;
    break;
  };

  last_result = f_lseek( fp, new_pos );
  new_pos = f_tell( fp );

  return last_result == FR_OK ? new_pos : VFS_RES_ERR;
}

static int32_t myfatfs_eof( const struct vfs_file *fd )
{
  GET_FIL_FP(fd);

  last_result = FR_OK;

  return f_eof( fp );
}

static int32_t myfatfs_tell( const struct vfs_file *fd )
{
  GET_FIL_FP(fd);

  last_result = FR_OK;

  return f_tell( fp );
}

static int32_t myfatfs_flush( const struct vfs_file *fd )
{
  GET_FIL_FP(fd);

  last_result = f_sync( fp );

  return last_result == FR_OK ? VFS_RES_OK : VFS_RES_ERR;
}

static uint32_t myfatfs_fsize( const struct vfs_file *fd )
{
  GET_FIL_FP(fd);

  last_result = FR_OK;

  return f_size( fp );
}

static int32_t myfatfs_ferrno( const struct vfs_file *fd )
{
  return -last_result;
}


// ---------------------------------------------------------------------------
// dir functions
//
#define GET_DIR_DP(descr) \
  const struct myvfs_dir *mydd = (const struct myvfs_dir *)descr; \
  DIR *dp = (DIR *)&(mydd->dp);

static int32_t myfatfs_closedir( const struct vfs_dir *dd )
{
  GET_DIR_DP(dd);

  last_result = f_closedir( dp );

  // free descriptor memory
  free( (void *)dd );

  return last_result == FR_OK ? VFS_RES_OK : VFS_RES_ERR;
}

static vfs_item *myfatfs_readdir( const struct vfs_dir *dd )
{
  GET_DIR_DP(dd);
  struct myvfs_item *di;

  if ((di = malloc( sizeof( struct myvfs_item ) ))) {
    FILINFO *fno = &(di->fno);

    if (FR_OK == (last_result = f_readdir( dp, fno ))) {
      // condition "no further item" is signalled with empty name
      if (fno->fname[0] != '\0') {
        di->vfs_item.fs_type = VFS_FS_FATFS;
        di->vfs_item.fns     = &myfatfs_item_fns;
        return (vfs_item *)di;
      }
    }
    free( di );
  }

  return NULL;
}


// ---------------------------------------------------------------------------
// dir info functions
//
#define GET_FILINFO_FNO(descr) \
  const struct myvfs_item *mydi = (const struct myvfs_item *)descr; \
  FILINFO *fno = (FILINFO *)&(mydi->fno);

static void myfatfs_iclose( const struct vfs_item *di )
{
  GET_FILINFO_FNO(di);
  (void)fno;

  // free descriptor memory
  free( (void *)di );
}

static uint32_t myfatfs_isize( const struct vfs_item *di )
{
  GET_FILINFO_FNO(di);

  return fno->fsize;
}

static int32_t myfatfs_time( const struct vfs_item *di, struct vfs_time *tm )
{
  GET_FILINFO_FNO(di);

  tm->year = (fno->fdate >>  9) + 1980;
  tm->mon  = (fno->fdate >>  5) & 0x0f;
  tm->day  =  fno->fdate        & 0x1f;
  tm->hour = (fno->ftime >> 11);
  tm->min  = (fno->ftime >>  5) & 0x3f;
  tm->sec  =  fno->ftime        & 0x3f;

  return VFS_RES_OK;
}

static const char *myfatfs_name( const struct vfs_item *di )
{
  GET_FILINFO_FNO(di);

  return fno->fname;
}

static int32_t myfatfs_is_dir( const struct vfs_item *di )
{
  GET_FILINFO_FNO(di);

  return fno->fattrib & AM_DIR ? 1 : 0;
}

static int32_t myfatfs_is_rdonly( const struct vfs_item *di )
{
  GET_FILINFO_FNO(di);

  return fno->fattrib & AM_RDO ? 1 : 0;
}

static int32_t myfatfs_is_hidden( const struct vfs_item *di )
{
  GET_FILINFO_FNO(di);

  return fno->fattrib & AM_HID ? 1 : 0;
}

static int32_t myfatfs_is_sys( const struct vfs_item *di )
{
  GET_FILINFO_FNO(di);

  return fno->fattrib & AM_SYS ? 1 : 0;
}

static int32_t myfatfs_is_arch( const struct vfs_item *di )
{
  GET_FILINFO_FNO(di);

  return fno->fattrib & AM_ARC ? 1 : 0;
}


// ---------------------------------------------------------------------------
// filesystem functions
//
static vfs_vol *myfatfs_mount( const char *name, int num )
{
  struct myvfs_vol *vol;

  // num argument specifies the physical driver
  if (num >= 0) {
    for (int i = 0; i < NUM_LOGICAL_DRIVES; i++) {
      if (0 == strncmp( name, volstr[i], strlen( volstr[i] ) )) {
        VolToPart[i].pd = num;
      }
    }
  }

  if ((vol = malloc( sizeof( struct myvfs_vol ) ))) {
    if ((vol->ldrname = strdup( name ))) {
      if (FR_OK == (last_result = f_mount( &(vol->fs), name, 1 ))) {
        vol->vfs_vol.fs_type = VFS_FS_FATFS;
        vol->vfs_vol.fns     = &myfatfs_vol_fns;
        return (vfs_vol *)vol;
      }
    }
  }

  if (vol) {
    if (vol->ldrname) free( vol->ldrname );
    free( vol );
  }
  return NULL;
}

static BYTE myfatfs_mode2flag( const char *mode )
{
  if (strlen( mode ) == 1) {
    if(strcmp( mode, "w" ) == 0)
      return FA_WRITE | FA_CREATE_ALWAYS;
    else if (strcmp( mode, "r" ) == 0)
      return FA_READ | FA_OPEN_EXISTING;
    else if (strcmp( mode, "a" ) == 0)
      return FA_WRITE | FA_OPEN_ALWAYS;
    else
      return FA_READ | FA_OPEN_EXISTING;
  } else if (strlen( mode ) == 2) {
    if (strcmp( mode, "r+" ) == 0)
      return FA_READ | FA_WRITE | FA_OPEN_EXISTING;
    else if (strcmp( mode, "w+" ) == 0)
      return FA_READ | FA_WRITE | FA_CREATE_ALWAYS;
    else if (strcmp( mode, "a+" ) ==0 )
      return FA_READ | FA_WRITE | FA_OPEN_ALWAYS;
    else
      return FA_READ | FA_OPEN_EXISTING;
  } else {
    return FA_READ | FA_OPEN_EXISTING;
  }
}

static vfs_file *myfatfs_open( const char *name, const char *mode )
{
  struct myvfs_file *fd;
  const BYTE flags = myfatfs_mode2flag( mode );

  if ((fd = malloc( sizeof( struct myvfs_file ) ))) {
    if (FR_OK == (last_result = f_open( &(fd->fp), name, flags ))) {
      // skip to end of file for append mode
      if (flags & FA_OPEN_ALWAYS)
        f_lseek( &(fd->fp), f_size( &(fd->fp) ) );

      fd->vfs_file.fs_type = VFS_FS_FATFS;
      fd->vfs_file.fns     = &myfatfs_file_fns;
      return (vfs_file *)fd;
    } else {
      free( fd );
    }
  }

  return NULL;
}

static vfs_dir *myfatfs_opendir( const char *name )
{
  struct myvfs_dir *dd;

  if ((dd = malloc( sizeof( struct myvfs_dir ) ))) {
    if (FR_OK == (last_result = f_opendir( &(dd->dp), name ))) {
      dd->vfs_dir.fs_type = VFS_FS_FATFS;
      dd->vfs_dir.fns     = &myfatfs_dir_fns;
      return (vfs_dir *)dd;
    } else {
      free( dd );
    }
  }

  return NULL;
}

static vfs_item *myfatfs_stat( const char *name )
{
  struct myvfs_item *di;

  if ((di = malloc( sizeof( struct myvfs_item ) ))) {
    if (FR_OK == (last_result = f_stat( name, &(di->fno) ))) {
      di->vfs_item.fs_type = VFS_FS_FATFS;
      di->vfs_item.fns     = &myfatfs_item_fns;
      return (vfs_item *)di;
    } else {
      free( di );
    }
  }

  return NULL;
}

static int32_t myfatfs_remove( const char *name )
{
  last_result = f_unlink( name );

  return last_result == FR_OK ? VFS_RES_OK : VFS_RES_ERR;
}

static int32_t myfatfs_rename( const char *oldname, const char *newname )
{
  last_result = f_rename( oldname, newname );

  return last_result == FR_OK ? VFS_RES_OK : VFS_RES_ERR;
}

static int32_t myfatfs_mkdir( const char *name )
{
  last_result = f_mkdir( name );

  return last_result == FR_OK ? VFS_RES_OK : VFS_RES_ERR;
}

static int32_t  myfatfs_fsinfo( uint32_t *total, uint32_t *used )
{
  DWORD free_clusters;
  FATFS *fatfs;

  if ((last_result = f_getfree( "", &free_clusters, &fatfs )) == FR_OK) {
    // provide information in kByte since uint32_t would clip to 4 GByte
    *total = (fatfs->n_fatent * fatfs->csize) / (1024 / _MAX_SS);
    *used  = *total - (free_clusters * fatfs->csize) / (1024 / _MAX_SS);
  }

  return last_result == FR_OK ? VFS_RES_OK : VFS_RES_ERR;
}

static int32_t myfatfs_chdrive( const char *name )
{
  last_result = f_chdrive( name );

  return last_result == FR_OK ? VFS_RES_OK : VFS_RES_ERR;
}

static int32_t myfatfs_chdir( const char *name )
{
  last_result = f_chdir( name );

  return last_result == FR_OK ? VFS_RES_OK : VFS_RES_ERR;
}

static int32_t myfatfs_errno( void )
{
  return -last_result;
}

static void myfatfs_clearerr( void )
{
  last_result = FR_OK;
}


// ---------------------------------------------------------------------------
// VFS interface functions
//
vfs_fs_fns *myfatfs_realm( const char *inname, char **outname, int set_current_drive )
{
  if (inname[0] == '/') {
    char *oname;

    // logical drive is specified, check if it's one of ours
    for (int i = 0; i < _VOLUMES; i++) {
      size_t volstr_len = strlen( volstr[i] );
      if (0 == strncmp( &(inname[1]), volstr[i], volstr_len )) {
        oname = strdup( inname );
        strcpy( oname, volstr[i] );
        oname[volstr_len] = ':';
        *outname = oname;

        if (set_current_drive) is_current_drive = true;
        return &myfatfs_fs_fns;
      }
    }
  } else {
    // no logical drive in patchspec, are we current drive?
    if (is_current_drive) {
      *outname = strdup( inname );
      return &myfatfs_fs_fns;
    }
  }

  if (set_current_drive) is_current_drive = false;
  return NULL;
}
