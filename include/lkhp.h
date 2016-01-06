/*****************************************************************************
 *
 * lkhp.h -- definitions for the lkhp module
 *
 * Copyright (C) 2002 Sriram S 
 *
 *****************************************************************************/

#ifndef _LKHP_H_
#define _LKHP_H_

#ifdef __KERNEL__
#include <linux/ioctl.h> /* _IOW etc stuff */
#include <linux/list.h>  /* linkedlist manipulation  */

/* version dependencies have been confined to a separate file */
#include "sysdep.h"

#define I386_LJMP_OPCODE 0xea

#define TRUE 1
#define FALSE 0
/*
 * Macros to help debugging
 */

#ifndef LKHP_MAJOR
#define LKHP_MAJOR 0   /* dynamic major by default */
#endif

typedef void * devfs_handle_t;  /* avoid #ifdef inside the structure */

extern devfs_handle_t lkhp_devfs_dir;

extern int lkhp_major;

extern struct file_operations lkhp_fops;        /* simplest: global */

ssize_t lkhp_read (struct file *filp, char *buf, size_t count,
                    loff_t *f_pos);
ssize_t lkhp_write (struct file *filp, const char *buf, size_t count,
                     loff_t *f_pos);
int     lkhp_ioctl (struct inode *inode, struct file *filp,
                     unsigned int cmd, unsigned long arg);

#endif /* __KERNEL__ */


typedef enum
{
    noError = 0,

    /* Warnings */
    generalWarning = 100,

    /* Errors */
    generalError = 200,
    patchNotFoundError,
    symbolNotFoundError,
    outOfCPUMemoryError,
    illegalOperationError,
} ErrCode;

typedef enum 
{
    ENABLED,
    DISABLED,
    ERROR
} HopState;

typedef struct HotPatch
{
#if __KERNEL__
    struct list_head list;               
#else
    void            *dummy[2];
#endif
    unsigned int    id;
    char            obj_file[64];       /* File used to load the patch  */
    char            symbol_name[64];    /* Symbol that got patched      */
    HopState        state;              /* ENABLED or DISABLED          */
    unsigned char   original_ins[7];    /* The original instruction     */
    unsigned char   patch_ins[7];       /* The patching instruction     */
    void            *hop_addr;          /* Address of replacement code  */
    void            *orig_addr;         /* Address of original code     */
    time_t          load_time;          /* Time this patch was loaded   */
    time_t          enable_time;        /* Time last enabled            */
    time_t          disable_time;       /* Time last disabled           */
} HotPatch;


/*
 * Ioctl definitions
 */

/* Use 'l' as magic number */
#define LKHP_IOC_MAGIC  'l'


/* ioctls */
#define LKHP_RESET               _IO(LKHP_IOC_MAGIC, 0)
#define LKHP_INSTALL_HOT_PATCH   _IOWR(LKHP_IOC_MAGIC, 1, int)
#define LKHP_GET_HOT_PATCH_INFO  _IOWR(LKHP_IOC_MAGIC, 2, int)
#define LKHP_REMOVE_HOT_PATCH    _IOWR(LKHP_IOC_MAGIC, 3, int)
#define LKHP_ENABLE_HOT_PATCH    _IOWR(LKHP_IOC_MAGIC, 4, int)
#define LKHP_DISABLE_HOT_PATCH   _IOWR(LKHP_IOC_MAGIC, 5, int)
#define LKHP_NUMBER_OF_PATCHES   _IOWR(LKHP_IOC_MAGIC, 6, int)
#define LKHP_GET_ALL_PATCH_INFO  _IOWR(LKHP_IOC_MAGIC, 7, int)

#define LKHP_IOCHARDRESET _IO(LKHP_IOC_MAGIC, 8) /* debugging tool */

/*
 * Structures used to transfer data between ioctls 
 */
typedef struct 
{
    HotPatch *hotPatch;
    ErrCode *err;
} InstallHotPatch_t;

typedef struct 
{
    int      id; 
    HotPatch *returnHotPatch;
    ErrCode *err;
} GetHotPatchInfo_t;

typedef struct 
{
    int     id; 
    ErrCode *err;
} RemoveHotPatch_t;

typedef struct 
{
    int id; 
    ErrCode *err;
} EnableHotPatch_t;

typedef struct 
{
    int id; 
    ErrCode *err;
} DisableHotPatch_t;


#ifdef __KERNEL__

/* Function Prototypes */
extern ErrCode InstallHotPatch(HotPatch *hotPatch);
extern ErrCode GetHotPatchInfo(int id, HotPatch **ReturnHotPatch);
extern ErrCode RemoveHotPatch(int id);
extern ErrCode EnableHotPatch(int id);
extern ErrCode DisableHotPatch(int id);

#endif /* __KERNEL__ */

#endif /* _LKHP_H */
