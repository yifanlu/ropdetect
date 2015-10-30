#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <linux/smp.h>
#define DRIVER_AUTHOR "Yifan Lu <yifanlu@stanford.edu>"
#define DRIVER_DESC   "ROP detection through pref monitor"

#define CPU0 0
#define CPU1 1
#define PMU_REGS_OFFSET 0x1000
#define PMU_REGS_SIZE 0x1000

#define PMU_PMCNTENSET 0xC00
#define PMU_PMLAR 0xFB0
#define PMU_PMCR 0xE04
#define PMU_PMCCNTR 0x07C

static phys_addr_t pmu_phys_base;
static struct resource *pmu_resource;
static void *pmu_regs;

static int init_ropdetect(void);
static void cleanup_ropdetect(void);

static void get_current_debug_regs(void *info)
{
  phys_addr_t base;
  int mpidr;

  asm("mrc p15,0,%0,c0,c0,5" : "=r" (mpidr));
  printk(KERN_DEBUG "Running on core %d\n", mpidr & 3);
  asm(//"mrc p14,0,%0,c1,c0,0\t\n" // disabled due to bug in BRCM CPU
      "mrc p14,0,%0,c2,c0,0\t\n"
      //"add %0,r4\t\n"
      "bic %0,#7\t\n" : "=r" (base) :: "memory"
    );
  *(phys_addr_t *)info = base;
}

static int init_ropdetect(void)
{
  phys_addr_t pmu_base;
  unsigned int pmcr;

  if (smp_call_function_single(CPU0, get_current_debug_regs, &pmu_phys_base, 1) < 0)
  {
    printk(KERN_ALERT "Unable to find debug regs for core 0\n");
    return -1;
  }

  pmu_base = pmu_phys_base+PMU_REGS_OFFSET;
  printk(KERN_DEBUG "PMU base: 0x%08X\n", pmu_base);
  if ((pmu_resource = request_mem_region(pmu_base, PMU_REGS_SIZE, "pmu")) == NULL)
  {
    printk(KERN_ALERT "Failed to request PMU memory region 0x%08X\n", pmu_base);
    return -1;
  }

  if ((pmu_regs = ioremap_nocache(pmu_base, PMU_REGS_SIZE)) == NULL)
  {
    printk(KERN_ALERT "Failed to map PMU memory region 0x%08X\n", pmu_base);
  }

  pmcr = ioread32(pmu_regs+PMU_PMCR);
  // unlock regs
  iowrite32(0xC5ACCE55, pmu_regs+PMU_PMLAR);
  // TODO: set up events here
  printk(KERN_DEBUG "Found %d event counters\n", (pmcr >> 11) & 0x1F);
  pmcr |= 0x27; // DP=1, X=0, D=0, C=1, P=1, E=1
  iowrite32(pmcr, pmu_regs+PMU_PMCR);
  // start counter
  iowrite32(0x80000000, pmu_regs+PMU_PMCNTENSET);
  for (int i = 0; i < 10000; i++); // wait a bit
  printk(KERN_DEBUG "Counts: 0x%08X\n", ioread32(pmu_regs+PMU_PMCCNTR));

  return 0;
}


static void cleanup_ropdetect(void)
{
  release_mem_region(pmu_phys_base+PMU_REGS_OFFSET, PMU_REGS_SIZE);
  iounmap(pmu_regs);
}


module_init(init_ropdetect);
module_exit(cleanup_ropdetect);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

