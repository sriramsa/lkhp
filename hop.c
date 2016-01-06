/*****************************************************************************
 *
 * hop.c - Command line interface
 *
 * Author: Sriram.S (sriram.achi@wipro.com)
 *
 * Usage:
 *      hop install
 *      hop remove
 *      hop list
 *      hop disable
 *      hop enable
 *
 *****************************************************************************/
#include <fcntl.h>      /* open     */ 
#include <unistd.h>     /* exit     */
#include <stdlib.h>     /* atoi     */
#include <string.h>     /* exit     */
#include <sys/ioctl.h>  /* ioctl    */
#include <errno.h>      /* errno    */
#include <time.h>       /* clock_t  */

#include <linux/module.h>  /* query_moudle() */
#include "include/lkhp.h"    

/* Device file descriptor */
int fd=-1;

/*
 * Function: GetKSymbolAddr
 *
 * Purpose:
 * Given the module name and symbol name within it, the routine returns the
 * kernel address of the data structure referenced by the symbol
 *
 * The routine returns 0 on success and error encountered in error conditions
 */

static ErrCode
getKSymbolAddr(char *ModuleName, char *SymbolName, unsigned int *Address)
{
    ErrCode err = noError;
    int ret = 0;
    int Size;
    int index;
    struct module_symbol *SymTable;

	/*
	 * Get the symbol table size of the module.
	 */
    ret = query_module(ModuleName, QM_SYMBOLS, NULL, 0, &Size);

    if ((ret == -1) && (errno != ENOSPC))
	{
        perror("query_module:1\n");
        return(symbolNotFoundError);
    }

    SymTable = (struct module_symbol *) malloc(Size);

    if ((ret = query_module(ModuleName, QM_SYMBOLS, SymTable, Size, &Size))) 
	{
        perror("query_module:2\n");
        return(symbolNotFoundError);
 	}

    for(index=0; index < Size; index++) 
	{
        char *extra;

        /* Remove the extra chars addes at the end of the symbol string */
        extra = strstr((char *)(SymTable[index].name+(unsigned long)SymTable), "_R");
        if (extra != NULL)
        {
            *extra = '\0';
        }

        if (!strcmp(SymbolName,
                (char *)(SymTable[index].name + (unsigned long) SymTable))) 
		{
            *Address = SymTable[index].value;
            free(SymTable);
            return(noError);
        }
    }
    free(SymTable);
    return(symbolNotFoundError);
}

/* Functions for the ioctl calls */
static HotPatch *ioctl_GetHotPatchInfo(int id)
{
  	int ret_val;
  	HotPatch *hotPatch;
  	GetHotPatchInfo_t userBuf;
  	ErrCode err= noError;;

  	hotPatch = (HotPatch *)malloc(sizeof(HotPatch));

  	userBuf.id = id;
  	userBuf.returnHotPatch = hotPatch;
  	userBuf.err = &err;

  	ret_val = ioctl(fd, LKHP_GET_HOT_PATCH_INFO, &userBuf);

  	if (ret_val < 0) {
    	perror ("ioctl_GetHotPatchInfo() failed:%d\n", ret_val);
    	free(hotPatch);
    	exit(-1);
  	}

  	/* If patch not found, then return NULL */
  	if (err == patchNotFoundError)
  	{
      	free(hotPatch);
      	return NULL;
  	}

  	return hotPatch;
}


static void ioctl_InstallHotPatch(char *symbol_name, char *file, unsigned long mod_addr)
{
    int ret_val;
    HotPatch hotPatch;
    InstallHotPatch_t userBuf;
    ErrCode err= 9;
    unsigned int Address;

    userBuf.err = &err;
    userBuf.hotPatch = &hotPatch;
    strcpy(hotPatch.obj_file, file);

    /* Get the kernel address */
    strcpy(hotPatch.symbol_name, symbol_name);

    err = getKSymbolAddr(NULL, &hotPatch.symbol_name[0], &Address);
    if (err != noError)
    {
        printf("ioctl_InstallHotPatch: getKSymbolAddr() kernel returned error\n");
        exit(-1);
    }

    userBuf.hotPatch->orig_addr = (void *)Address;
    userBuf.hotPatch->hop_addr = (void *)mod_addr;

    /* Make the ioctl to the driver */
    ret_val = ioctl(fd, LKHP_INSTALL_HOT_PATCH, &userBuf);

    if (ret_val < 0) {
        perror ("ioctl_InstallHotPatch() failed:%d\n", ret_val);
        exit(-1);
    }
  
    if (err == outOfCPUMemoryError)
    {
        printf("ERROR: Out of CPU Memory.\n");
    	exit(-1);
    }

    return;
}

static void ioctl_RemoveHotPatch(int id)
{
  	int ret_val;
  	RemoveHotPatch_t userBuf;
  	ErrCode err= noError;;

  	userBuf.id = id;
  	userBuf.err = &err;

  	ret_val = ioctl(fd, LKHP_REMOVE_HOT_PATCH, &userBuf);

  	if (ret_val < 0) {
    	perror ("ioctl_RemoveHotPatch() failed:%d\n", ret_val);
    	exit(-1);
  	}

  	if (err == patchNotFoundError)
  	{
      	printf("ERROR: Patch id not found.\n");
    	exit(-1);
  	}

  	return;
}

static void ioctl_EnableHotPatch(int id)
{
    int ret_val;
    RemoveHotPatch_t userBuf;
    ErrCode err= noError;;

    userBuf.id = id;
    userBuf.err = &err;

    ret_val = ioctl(fd, LKHP_ENABLE_HOT_PATCH, &userBuf);

    if (ret_val < 0) 
    {
        perror ("ioctl_EnableHotPatch() failed:%d\n", ret_val);
        exit(-1);
    }

    if (err == patchNotFoundError)
    {
        printf("ERROR: Patch id not found.\n");
        exit(-1);
    }
    else if (err == illegalOperationError)
    {
        printf("ERROR: Patch already enabled.\n");
        exit(-1);
    }
    return;
}

static void ioctl_DisableHotPatch(int id)
{
    int ret_val;
    RemoveHotPatch_t userBuf;
    ErrCode err= noError;;

    userBuf.id = id;
    userBuf.err = &err;

    ret_val = ioctl(fd, LKHP_DISABLE_HOT_PATCH, &userBuf);
    
    if (ret_val < 0) 
    {
        perror ("ioctl_DisableHotPatch() failed:%d\n", ret_val);
        exit(-1);
    }

    if (err == patchNotFoundError)
    {
        printf("ERROR: Patch id not found.\n");
        exit(-1);
    }
    else if (err == illegalOperationError)
    {
        printf("ERROR: Patch already disabled.\n");
        exit(-1);
    }
    return;
}

static void ioctl_ListHotPatches()
{
    int ret_val, i;    
    int numPatches;
    HotPatch *hotPatch;
    void *buf;

    /* Get the number of patches */
    ret_val = ioctl(fd, LKHP_NUMBER_OF_PATCHES, &numPatches);
    if (ret_val < 0) 
    {
        perror ("ioctl_ListHotPatches() failed:%d\n", ret_val);
        exit(-1);
    }

    /* Allocate buffer to store patch data */
    buf = malloc(sizeof(HotPatch) * numPatches);

	if (buf == NULL)
	{
		printf("ERROR: Not enough CPU memory for the operation");
		exit(-1);
	}

    ret_val = ioctl(fd, LKHP_GET_ALL_PATCH_INFO, buf);
    if (ret_val < 0) 
    {
        perror ("ioctl_ListHotPatches() failed:%d\n", ret_val);
        exit(-1);
    }

    hotPatch = (HotPatch *)buf;
    printf(" ID   Symbol Name   Status    Orig Address   Rep Address   Patch\n");
    for (i=0; i<numPatches; i++)
    {
        printf("%3d   %11s   %8s %11p     %10p    %s \n", 
                hotPatch[i].id, hotPatch[i].symbol_name,  
                (hotPatch[i].state == 0)?"Enabled ":"Disabled", 
                hotPatch[i].orig_addr, hotPatch[i].hop_addr, 
                hotPatch[i].obj_file);
    }

    free((void *)hotPatch);
}

static void printUsage(const char *name)
{
    printf("Usage: %s <install|remove|disable|enable|list> <id>\n", name);
    exit(-1);
}

/* 
 * Enum for identifying the command supplied by the user 
 */
typedef enum 
{
    install=1, remove, enable, disable, list
} UserChoice;


/* 
 * Makes proper ioctls to the driver.
 */
int main(int argc, char *argv[])
{
    int ret_val;
    unsigned int id;
    UserChoice choice;

    if (argc < 2)
    {
        printUsage(argv[0]);
    }

    if (strcmp(argv[1], "install") == 0)
    {
        if (argc < 4)
            printUsage(argv[0]);
        choice = install;
    }
    else if (strcmp(argv[1], "remove") == 0)
    {
        if (argc > 2)
            id = atoi(argv[2]);
        choice = remove;
    }
    else if (strcmp(argv[1], "enable") == 0)
    {
        if (argc > 2)
            id = atoi(argv[2]);
        choice = enable;
    }
    else if (strcmp(argv[1], "disable") == 0)
    {
        if (argc > 2)
            id = atoi(argv[2]);
        choice = disable;
    }
    else if (strcmp(argv[1], "list") == 0)
    {
        choice = list;
    }
    else
    {
        printUsage(argv[0]);
    }


    fd = open("/dev/lkhp", 0);

    if (fd < 0) {
        perror ("Can't open device file: /dev/lkhp\n");
        exit(-1);
    }


    switch (choice)
    {
        case install:
            {
                unsigned long hop_addr;

                sscanf (argv[4], "%x", &hop_addr);
#if DEBUG_LKHP
                printf("address = %x\n", hop_addr);
#endif

                ioctl_InstallHotPatch(argv[3], argv[2], hop_addr);
            }
            break;
        case remove:
            {
                HotPatch *hotPatch;
                char mod_name[32];

                /* Get the information about the patch being removed */
                hotPatch = ioctl_GetHotPatchInfo(id);

                if (hotPatch == NULL)
                {
                    printf("ERROR: Patch id not found\n");
                    exit(-1);
                }
                
                /* Remove the patch */
                ioctl_RemoveHotPatch(id);

                /* 
                 * Remove the module from the system. For whatever reason,
                 * release_module() has been removed from the distribution. So
                 * had to fork and exec to rmmod to remove the module
                 */
                strcpy(mod_name, hotPatch->symbol_name); 
                if (fork() == 0)
                {
                    /* Child process */
                    execlp("rmmod", "rmmod", mod_name, NULL);
                }

                free(hotPatch);
            }
            break;
        case enable:
            ioctl_EnableHotPatch(id);
            break;
        case disable:
            ioctl_DisableHotPatch(id);
            break;
        case list:
            /* If id is specified */
            if (argc == 3)
            {
                int id;
                struct tm *gmt;
                HotPatch *hotPatch;
                char times[18];
           
                sscanf(argv[2], "%d", &id);
                
                /* Get all the information on the patch */
                hotPatch = ioctl_GetHotPatchInfo(id);
                if (hotPatch == NULL)
                {
                    printf("ERROR: Patch id not found\n");
                    exit(-1);
                }
                
                /*
                 * Print the hot patch info 
                 */
                printf("Patch ID        : %d\t\tSymbol       : %s\n", 
                                hotPatch->id, hotPatch->symbol_name);

                /* Format the time string */
                gmt = (struct tm *)gmtime(&hotPatch->load_time);
                strftime(times, 18, "%T %D", gmt);
                printf("State           : %s\tPatched at   : %s\n", 
                                (hotPatch->state == ENABLED)?"Enabled":"Disabled",
                                times);
                gmt = (struct tm *)gmtime(&hotPatch->enable_time);
                strftime(times, 18, "%T %D", gmt);
                printf("Original Address: %p\tLast Enabled : %s\n", 
                                hotPatch->orig_addr, 
                                times);
                printf("Patched  Address: %p\tLast Disabled: ", hotPatch->hop_addr);

                if (hotPatch->disable_time != (time_t)0)
                {
                    gmt = (struct tm *)gmtime(&hotPatch->disable_time);
                    strftime(times, 18, "%T %D", gmt);
                    printf("%s\n", times);
                }
                else
                {
                    printf("never disabled\n");
                }

                printf("Patch file      : %s\n", hotPatch->obj_file);
                printf("Patch Module    : %s\n", hotPatch->symbol_name);
                free(hotPatch);
            }
            else
            {
                /* List all the patches */
                ioctl_ListHotPatches();
            }
            break;
    }

    close(fd);

    exit(0);
}
