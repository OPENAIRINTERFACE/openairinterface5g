#include "defs.h"
#include "linux/module.h"

#ifdef BIGPHYSAREA
#ifdef ARCH_64
char *bigphys_ptr,*bigphys_current;
#else //ARCH_64
unsigned int bigphys_ptr,bigphys_current;
#endif //ARCH_64

// return pointer to memory in big physical area aligned to 16 bytes

void* bigphys_malloc(int n)
{



  int n2 = n + ((16-(n%16))%16);
#ifdef ARCH_64
  char *bigphys_old;
#else
  unsigned int bigphys_old;
#endif

  printk("[BIGPHYSAREA] Calling bigphys_malloc for %d (%d) bytes\n",n,n2);

#ifdef ARCH_64
  printk("[BIGPHYSAREA] Allocated Memory @ %p\n",bigphys_current);
#endif
  bigphys_old = bigphys_current;
  bigphys_current += n2;

#ifdef ARCH_64
  printk("[BIGPHYSAREA] Allocated Memory top now @ %p\n",bigphys_current);
  return ((void *)(bigphys_old));
#else //ARCH_64
  //printk("[BIGPHYSAREA] Allocated Memory %d\n",bigphys_current-bigphys_ptr);
  return ((void *)(bigphys_old));
#endif //ARCH_64
}

EXPORT_SYMBOL(bigphys_malloc);
#endif



