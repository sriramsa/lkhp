/****************************************************************************
 *
 * dummy.c -- The dummy driver to load the patch functions.
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

#define MODVERSIONS
#include <linux/config.h>
#include <linux/module.h>

#include <linux/kernel.h>   /* printk() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/types.h>    /* O_ACCMODE */
#include <linux/init.h>    /* O_ACCMODE */

#include <asm/system.h>     /* cli(), *_flags */
void lkhp_cleanup_module(void);
int lkhp_init_module(void);

int lkhp_release(struct inode *inode, struct file *filp)
{
    MOD_DEC_USE_COUNT;
    return 0;
}

struct file_operations lkhp_fops = {
    read:       NULL,
    write:      NULL,
    ioctl:      NULL,
    open:       NULL,
    release:    lkhp_release,
};

void lkhp_cleanup_module(void)
{
    inter_module_unregister((THIS_MODULE)->name);
    return;
}

int lkhp_init_module(void)
{
    struct module *mod;
    SET_MODULE_OWNER(&lkhp_fops);

    mod = THIS_MODULE ;

    /* The address of this module needs to be passed to the
     * master function. It is done by registering an inter module
     * communication.  */
    inter_module_register((THIS_MODULE)->name, THIS_MODULE, THIS_MODULE);

    MOD_INC_USE_COUNT;
    EXPORT_NO_SYMBOLS;

    return 0;
}

module_init(lkhp_init_module);
module_exit(lkhp_cleanup_module);
