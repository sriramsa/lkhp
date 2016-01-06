/***************************************************************************
 *
 * lkhp.c: Contains all the lkhp functions for ioctls
 *
 ***************************************************************************/

#include <linux/kernel.h>
#include <linux/malloc.h>   /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/list.h>     /* Linked list... */
#include <linux/module.h>   /* inter module comm.. */

#include <asm/system.h>     /* cli(), *_flags */

#include "lkhp.h"

extern HotPatch *listHead;

static HotPatch *findHotPatchNode(int id)
{
    HotPatch *returnHotPatch = NULL;
    struct list_head *ptr;

    for (ptr = listHead->list.next; ptr != &listHead->list; ptr = ptr->next)
    {
       returnHotPatch = list_entry(ptr, HotPatch, list);
       if (returnHotPatch->id == id)
           break;
       else
           returnHotPatch = NULL;
    }
    return returnHotPatch;
}

/* 
 * Debug function 
 */
#if DEBUG_LKHP
static void PrintNodes()
{
    HotPatch *returnHotPatch = NULL;
    struct list_head *ptr;

    printk("Patches in list\n");
    printk("id  Status\n");
    for (ptr = listHead->list.next; ptr != &listHead->list; ptr = ptr->next)
    {
       returnHotPatch = list_entry(ptr, HotPatch, list);
       printk("%d  %d\n", returnHotPatch->id, returnHotPatch->state);
    }
}

#endif

/*
 * Install a new hot patch.
 */
ErrCode InstallHotPatch(HotPatch *hotPatch)
{
    static unsigned int id=0;
    char *orig_addr, *hop_addr;
    pgd_t *op_pgd;
    pmd_t *op_pmd;
    pte_t *op_pte;
    register unsigned long flags=0;
    int pageReset = FALSE;

    /* Assign a new patch id */
    hotPatch->id = ++id;

    hotPatch->state = ENABLED;
    hotPatch->load_time = hotPatch->enable_time = CURRENT_TIME;
    hotPatch->disable_time = (time_t)0;

    /* Add to the list */
    list_add_tail(&hotPatch->list, &listHead->list);

    /* Obtain both the original and hot patch addresses */
    orig_addr = (char *)hotPatch->orig_addr;
    hop_addr = (char *)hotPatch->hop_addr;

#if DEBUG_LKHP
    printk("original addr = %p\n", orig_addr);
    printk("HotPatch addr = %p\n", hop_addr);
#endif

    /*
     * Construct the OP code for the jump.
     * opcode:
     * ljmp $__KERNEL_CS, <32-bit address>
     * op code for far jump 'ljmp' is 0xea 
     */
    hotPatch->patch_ins[0] = I386_LJMP_OPCODE;   

    /* Write the address in little endian format */
    *((unsigned int *)&hotPatch->patch_ins[1]) = (unsigned int)hop_addr; 

    /* Kernel Code Segment start address */
    *((unsigned short *)&hotPatch->patch_ins[5]) = __KERNEL_CS;

    printk("HotPatch opcode = %x %x %x %x %x %x %x\n", hotPatch->patch_ins[0], hotPatch->patch_ins[1], hotPatch->patch_ins[2], hotPatch->patch_ins[3], hotPatch->patch_ins[4], hotPatch->patch_ins[5], hotPatch->patch_ins[6]);

    /* Save the old instruction(s) */
    hotPatch->original_ins[0] = *orig_addr;
    hotPatch->original_ins[1] = *(orig_addr+1);
    hotPatch->original_ins[2] = *(orig_addr+2);
    hotPatch->original_ins[3] = *(orig_addr+3);
    hotPatch->original_ins[4] = *(orig_addr+4);
    hotPatch->original_ins[5] = *(orig_addr+5);
    hotPatch->original_ins[6] = *(orig_addr+6);

    /* Obtain the page table entry for the target address */
    /* Get the kernel page directory entry pointer */
    op_pgd = pgd_offset_k((unsigned long)orig_addr);
    if (op_pgd == NULL)
    {
        restore_flags(flags);
        sti();
        return generalError;
    }

    /* Get the page middle directory entry pointer */
    op_pmd = pmd_offset(op_pgd, (unsigned long)orig_addr);
    if (op_pmd == NULL)
    {
        restore_flags(flags);
        sti();
        return generalError;
    }

    /* Get the page table entry pointer */
    op_pte = pte_offset(op_pmd, (unsigned long)orig_addr);
    if (op_pte == NULL)
    {
        restore_flags(flags);
        sti();
        return generalError;
    }

    /* Disable interrupts */
    save_flags(flags);
    cli();

    /* If read only flag is set */
    if (! pte_write(*op_pte))
    {
        /* Clear the Read Write flag */
        set_pte(op_pte, pte_mkwrite(*op_pte));

        /* Page table entry is modified, flush the corresponding TLB entry */
        flush_tlb();
        pageReset = TRUE;
    }

    
    /* Read the original OP code(7 bytes) that is going to be replaced */
    /* Patch with the OP code. No need to flush the instruction cache */
    *orig_addr =  hotPatch->patch_ins[0];
    *(orig_addr+1) = hotPatch->patch_ins[1];
    *(orig_addr+2) = hotPatch->patch_ins[2];
    *(orig_addr+3) = hotPatch->patch_ins[3];
    *(orig_addr+4) = hotPatch->patch_ins[4];
    *(orig_addr+5) = hotPatch->patch_ins[5];
    *(orig_addr+6) = hotPatch->patch_ins[6];

    /* Reset the page bits back */
    if (pageReset == TRUE)
    {
        set_pte(op_pte, pte_mkread(*op_pte));
        flush_tlb();
    }

    /* Enable interrupts */
    restore_flags(flags);
    sti();
    
    return noError;
}

/*
 * Return the hot patch info requested.
 */
ErrCode GetHotPatchInfo(int id, HotPatch **ReturnHotPatch)
{
    HotPatch *hotPatch = NULL;

    /* Traverse the list and find the node */
    hotPatch = findHotPatchNode(id);

    /* If not found */
    if (hotPatch == NULL)
        return patchNotFoundError;

    *ReturnHotPatch = hotPatch;
    return noError;
}

/*
 * Remove the hot patch permanently.
 */
ErrCode RemoveHotPatch(int id)
{
    char *orig_addr;
    HotPatch *hotPatch=NULL;
    struct module *mod;

    printk("Removing HotPatch id = %d\n", id);

    /* Traverse the list and find the node */
    hotPatch = findHotPatchNode(id);

    /* If not found */
    if (hotPatch == NULL)
        return patchNotFoundError;

    /* Delete the node from the list */
    list_del(&hotPatch->list);

    if (hotPatch->state == ENABLED)
    {
        /* Write the original instructions back */
        orig_addr = (char *)hotPatch->orig_addr;

        *orig_addr     = hotPatch->original_ins[0];
        *(orig_addr+1) = hotPatch->original_ins[1];
        *(orig_addr+2) = hotPatch->original_ins[2];
        *(orig_addr+3) = hotPatch->original_ins[3];
        *(orig_addr+4) = hotPatch->original_ins[4];
        *(orig_addr+5) = hotPatch->original_ins[5];
        *(orig_addr+6) = hotPatch->original_ins[6];
    }


    /* Get the module pointer from the module being removed */
    mod = (struct module *)inter_module_get(hotPatch->symbol_name);

    /* Decrease the usage count of the module being removed. This
     * will enable it to get removed from the system. Decrease it 2 times
     * since the call to inter_module_get() has increased the usage coung */
    __MOD_DEC_USE_COUNT(mod);
    __MOD_DEC_USE_COUNT(mod);

    /* Free the node */
    kfree((void *)hotPatch);

    return noError;
}

/*
 * Enable a disabled hot patch.
 */
ErrCode EnableHotPatch(int id)
{
    char *orig_addr;
    HotPatch *hotPatch=NULL;

    printk("Enabling HotPatch id = %d\n", id);

    /* Traverse the list and find the node */
    hotPatch = findHotPatchNode(id);

    /* If not found */
    if (hotPatch == NULL)
        return patchNotFoundError;

    if (hotPatch->state == ENABLED)
    {
        return illegalOperationError;
    }

    /* Change the state of the node */
    hotPatch->state = ENABLED;
    hotPatch->enable_time = CURRENT_TIME;

    /* Write the hot patch instructions back */
    orig_addr = (char *)hotPatch->orig_addr;

    *orig_addr = hotPatch->patch_ins[0];
    *(orig_addr+1) = hotPatch->patch_ins[1];
    *(orig_addr+2) = hotPatch->patch_ins[2];
    *(orig_addr+3) = hotPatch->patch_ins[3];
    *(orig_addr+4) = hotPatch->patch_ins[4];
    *(orig_addr+5) = hotPatch->patch_ins[5];
    *(orig_addr+6) = hotPatch->patch_ins[6];

    return noError;
}

/*
 * Temporarily disable a hot patch.
 */
ErrCode DisableHotPatch(int id)
{
    char *orig_addr;
    HotPatch *hotPatch=NULL;

    printk("Disabling HotPatch id = %d\n", id);

    /* Traverse the list and find the node */
    hotPatch = findHotPatchNode(id);

    /* If not found */
    if (hotPatch == NULL)
        return patchNotFoundError;

    if (hotPatch->state == DISABLED)
    {
        return illegalOperationError;
    }

    /* Change the state of the node */
    hotPatch->state = DISABLED;
    hotPatch->disable_time = CURRENT_TIME;

    /* Write the original instructions back */
    orig_addr = (char *)hotPatch->orig_addr;

    *orig_addr     = hotPatch->original_ins[0];
    *(orig_addr+1) = hotPatch->original_ins[1];
    *(orig_addr+2) = hotPatch->original_ins[2];
    *(orig_addr+3) = hotPatch->original_ins[3];
    *(orig_addr+4) = hotPatch->original_ins[4];
    *(orig_addr+5) = hotPatch->original_ins[5];
    *(orig_addr+6) = hotPatch->original_ins[6];

    return noError;
}
