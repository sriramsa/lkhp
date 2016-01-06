/****************************************************************************
 *
 * main.c -- The Main lkhp character module
 *
 * Copyright (C) 2001 Sriram S
 *
 ****************************************************************************/

#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/config.h>
#include <linux/module.h>

#include <linux/kernel.h>   /* printk() */
#include <linux/malloc.h>   /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */

#include <asm/system.h>     /* cli(), *_flags */

#include "lkhp.h"          /* local definitions */

/*
 * I don't use static symbols here, because we export no symbols
 */

int lkhp_major =   LKHP_MAJOR;

MODULE_PARM(lkhp_major,"i");
MODULE_AUTHOR("Sriram S");

HotPatch *listHead;
unsigned int numPatches=0;

/*
 * Open and close
 */

/* In lkhp_open, the fop_array is used according to TYPE(dev) */
int lkhp_open(struct inode *inode, struct file *filp)
{

    MOD_INC_USE_COUNT;  /* Before we maybe sleep */

    return 0;          /* success */
}

int lkhp_release(struct inode *inode, struct file *filp)
{
    MOD_DEC_USE_COUNT;
    return 0;
}

/*
 * Data management: read and write
 */

ssize_t lkhp_read(struct file *filp, char *buf, size_t count,
                loff_t *f_pos)
{
    return 0;
}

ssize_t lkhp_write(struct file *filp, const char *buf, size_t count,
                loff_t *f_pos)
{
    return 0;
}


/*
 * The ioctl() implementation
 *
 */

int lkhp_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{

    ErrCode err = noError;
    int ret = 0;
   
    /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != LKHP_IOC_MAGIC) return -ENOTTY;

    /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err =  !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
    if (err) return -EFAULT;

    switch(cmd) 
    {
        case LKHP_RESET:
            break;
        case LKHP_INSTALL_HOT_PATCH:
            printk("LKHP_INSTALL_HOT_PATCH\n");
            {
                InstallHotPatch_t kernBuf;
                HotPatch *hotPatch;

                /* Copy the arguements from user space */
                copy_from_user(&kernBuf, (void *)arg, sizeof(InstallHotPatch_t));

                hotPatch = (HotPatch *)kmalloc(sizeof(HotPatch), GFP_KERNEL);

                if (hotPatch != NULL)
                {
                    copy_from_user(hotPatch, (void *)kernBuf.hotPatch, sizeof(HotPatch));
                    
                    err = InstallHotPatch(hotPatch);
                    if (err == noError)
                        numPatches++;
                }
                else
                {
                    err = outOfCPUMemoryError;
                }

                /* Copy the error back to user space */
                copy_to_user((void *)kernBuf.err, (void *)&err, sizeof(ErrCode));
            }
            break;
        case LKHP_GET_HOT_PATCH_INFO:
            printk("LKHP_GET_HOT_PATCH_INFO\n");
            {
                GetHotPatchInfo_t kernBuf;
                HotPatch *hotPatch;

                /* 
                 * Copy the arguements from user space 
                 */
                copy_from_user(&kernBuf, (void *)arg, sizeof(GetHotPatchInfo_t));

                err = GetHotPatchInfo(kernBuf.id, &hotPatch);

                /* 
                 * If noError Copy the Hot patch structure back to user space 
                 */
                if (err == noError)
                {
                    
                    copy_to_user((void *)kernBuf.returnHotPatch, (void *)hotPatch, sizeof(HotPatch));
                }

                /* Copy te error back to user space */
                copy_to_user((void *)kernBuf.err, (void *)&err, sizeof(ErrCode));
            }
            break;
        case LKHP_REMOVE_HOT_PATCH:
            printk("LKHP_REMOVE_HOT_PATCH\n");
            {
                RemoveHotPatch_t kernBuf;

                /* Copy the arguements from user space */
                copy_from_user(&kernBuf, (void *)arg, sizeof(RemoveHotPatch_t));

                err = RemoveHotPatch(kernBuf.id);
                if (err == noError)
                    numPatches--;

                /* Copy the error back to user space */
                copy_to_user((void *)kernBuf.err, (void *)&err, sizeof(ErrCode));
            }
            break;
        case LKHP_ENABLE_HOT_PATCH:
            printk("LKHP_ENABLE_HOT_PATCH\n");
            {
                EnableHotPatch_t kernBuf;

                /* Copy the arguements from user space */
                copy_from_user(&kernBuf, (void *)arg, sizeof(EnableHotPatch_t));

                err = EnableHotPatch(kernBuf.id);
                
                /* Copy te error back to user space */
                copy_to_user((void *)kernBuf.err, (void *)&err, sizeof(ErrCode));
            }
            break;
        case LKHP_DISABLE_HOT_PATCH:
            printk("LKHP_DISABLE_HOT_PATCH\n");
            {
                DisableHotPatch_t kernBuf;

                /* Copy the arguements from user space */
                copy_from_user(&kernBuf, (void *)arg, sizeof(DisableHotPatch_t));

                err = DisableHotPatch(kernBuf.id);
                
                /* Copy te error back to user space */
                copy_to_user((void *)kernBuf.err, (void *)&err, sizeof(ErrCode));
            }
            break;
        case LKHP_NUMBER_OF_PATCHES:
            {
                /* Copy the number of patches to the user */
                copy_to_user((void *)arg, (void *)&numPatches, sizeof(int));
            }
            break;
        case LKHP_GET_ALL_PATCH_INFO:
            {
                HotPatch *hotPatch;
                struct list_head *ptr;
                unsigned int i=0;

                /* Copy the whole list into the buffer */
                for (ptr = listHead->list.next; ptr != &listHead->list; ptr = ptr->next, i++)
                {
                    hotPatch = list_entry(ptr, HotPatch, list);
                    copy_to_user((void *)(arg+(i*sizeof(HotPatch))), (void *)hotPatch, sizeof(HotPatch));
                }
            }
            break;
        default:  
            return -ENOTTY;
    }
    return ret;

}

struct file_operations lkhp_fops = {
    read:       lkhp_read,
    write:      lkhp_write,
    ioctl:      lkhp_ioctl,
    open:       lkhp_open,
    release:    lkhp_release,
};

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void lkhp_cleanup_module(void)
{
    HotPatch *hotPatch;
    struct list_head *ptr;

    /* Free the HotPatch nodes linked list */
    ptr = &listHead->list; 
    do
    {
       hotPatch = list_entry(ptr, HotPatch, list);
       ptr=ptr->next;
       kfree((void *)hotPatch);
    } while(ptr != &listHead->list);

    /* cleanup_module is never called if registering failed */
    unregister_chrdev(lkhp_major, "lkhp");

}


int lkhp_init_module(void)
{
    int result;

    SET_MODULE_OWNER(&lkhp_fops);

    /*
     * Register your major, and accept a dynamic number. This is the
     * first thing to do, in order to avoid releasing other module's
     * fops in lkhp_cleanup_module()
     */
    result = register_chrdev(lkhp_major, "lkhp", &lkhp_fops);
    if (result < 0) {
        printk(KERN_WARNING "lkhp: can't get major %d\n",lkhp_major);
        return result;
    }
    if (lkhp_major == 0) lkhp_major = result; /* dynamic */

    EXPORT_NO_SYMBOLS; /* otherwise, leave global symbols visible */

    /* Do linked list initialization */
    listHead = (HotPatch *)kmalloc(sizeof(HotPatch), GFP_KERNEL);
    listHead->id = -1;
    INIT_LIST_HEAD (&listHead->list);

    numPatches = 0;
    return 0; /* succeed */

//errout:
    lkhp_cleanup_module();
    return result;
}

module_init(lkhp_init_module);
module_exit(lkhp_cleanup_module);
