#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#define DRIVER_AUTHOR "Yifan Lu <yifanlu@stanford.edu>"
#define DRIVER_DESC   "ROP detection through pref monitor"

static int init_ropdetect(void);
static void cleanup_ropdetect(void);

static phys_addr_t get_current_debug_regs(void)
{
  phys_addr_t base;
  asm ("mrc p14,0,%0,c1,c0,0\t\n"
       "mrc p14,0,r4,c2,c0,0\t\n"
       "add %0,r4\t\n"
       "bic %0,#7\t\n" : "=r" (base) :: "r4", "memory"
      );
  return base;
}

static int init_ropdetect(void)
{
   printk(KERN_ALERT "Hello, world: 0x%08X\n", get_current_debug_regs());
   return -1;
}


static void cleanup_ropdetect(void)
{
   printk(KERN_ALERT "Goodbye, world 4\n");
}


module_init(init_ropdetect);
module_exit(cleanup_ropdetect);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

