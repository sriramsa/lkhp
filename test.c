 
#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <time.h>  


#include "include/lkhp.h"    

int fd=-1;
/* Functions for the ioctl calls */

ioctl_GetHotPatchInfo()
{
  int ret_val;
  HotPatch hotPatch;
  GetHotPatchInfo_t userBuf;
  ErrCode err= noError;;

  printf("test.c: ioctl_GetHotPatchInfo() \n");
  hotPatch.id = 100;
  userBuf.returnHotPatch = &hotPatch;
  userBuf.err = &err;

  ret_val = ioctl(fd, LKHP_GET_HOT_PATCH_INFO, &userBuf);

  if (ret_val < 0) {
    perror ("ioctl_GetHotPatchInfo() failed:%d\n", ret_val);
    exit(-1);
  }
  printf("hotPatch.id =  %d\n", hotPatch.id);
  printf("hotPatch.state =  %d\n", hotPatch.state);
  printf("userBuf.err =  %d\n", *(userBuf.err));
}


ioctl_InstallHotPatch()
{
  int ret_val;
  HotPatch hotPatch;
  InstallHotPatch_t userBuf;
  ErrCode err= 9;

  printf("test.c: ioctl_InstallHotPatch() \n");
  hotPatch.id = 100;
  userBuf.hotPatch = &hotPatch;
  userBuf.err = &err;

  printf("&err = %p\n", &err);
  ret_val = ioctl(fd, LKHP_INSTALL_HOT_PATCH, &userBuf);

  if (ret_val < 0) {
    perror ("ioctl_InstallHotPatch() failed:%d\n", ret_val);
    exit(-1);
  }
  printf("userBuf.err =  %d\n", *(userBuf.err));
}

ioctl_RemoveHotPatch(int id)
{
  int ret_val;
  RemoveHotPatch_t userBuf;
  ErrCode err= noError;;

  printf("test.c: ioctl_RemoveHotPatch() \n");
  userBuf.id = id;
  userBuf.err = &err;

  ret_val = ioctl(fd, LKHP_REMOVE_HOT_PATCH, &userBuf);
  //ret_val = ioctl(fd, LKHP_DISABLE_HOT_PATCH, &userBuf);
  //ret_val = ioctl(fd, LKHP_ENABLE_HOT_PATCH, &userBuf);

  if (ret_val < 0) {
    perror ("ioctl_RemoveHotPatch() failed:%d\n", ret_val);
    exit(-1);
  }
  printf("userBuf.err =  %d\n", *(userBuf.err));
}

/* Main - Call the ioctl functions */
main(int argc, char *argv[])
{
  int ret_val;
  char *msg = "Message passed by ioctl\n";

  fd = open("/dev/lkhp", 0);

  if (fd < 0) {
    perror ("Can't open device file: /dev/lkhp\n");
    exit(-1);
  }

  ioctl_RemoveHotPatch(3);
#if 0
  ioctl_InstallHotPatch();
  ioctl_GetHotPatchInfo();
  ioctl_EnableHotPatch();
  ioctl_DisableHotPatch();
#endif
  close(fd);
}
