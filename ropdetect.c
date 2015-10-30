#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#define DRIVER_AUTHOR "Yifan Lu <yifanlu@stanford.edu>"
#define DRIVER_DESC   "ROP detection through pref monitor"

static int init_ropdetect(void);
static void cleanup_ropdetect(void);


static int init_ropdetect(void)
{
   printk(KERN_ALERT "Hello, world 4\n");
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

