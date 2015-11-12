#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kthread.h>
#include <asm/io.h>
#include <linux/smp.h>
#include "ropdetect.h"

#define DRIVER_AUTHOR "Yifan Lu <yifanlu@stanford.edu>"
#define DRIVER_DESC   "ROP detection through pref monitor"

#define CPU_TARGET 0
#define CPU_MONITOR 1
#define PMU_REGS_OFFSET 0x1000
#define PMU_REGS_SIZE 0x1000

#define PMU_PMXEVCNTR0 0x000
#define PMU_PMXEVCNTR1 0x004
#define PMU_PMXEVCNTR2 0x008
#define PMU_PMXEVCNTR3 0x00C
#define PMU_PMCCNTR 0x07C
#define PMU_PMXEVTYPER0 0x400
#define PMU_PMXEVTYPER1 0x404
#define PMU_PMXEVTYPER2 0x408
#define PMU_PMXEVTYPER3 0x40C
#define PMU_PMCNTENSET 0xC00
#define PMU_PMCNTENCLR 0xC20
#define PMU_PMOVSR 0xC80
#define PMU_PMLAR 0xFB0
#define PMU_PMCR 0xE04

// stolen from: arch/arm/kernel/perf_event_v7.c
/* Common ARMv7 event types */
enum armv7_perf_types {
  ARMV7_PERFCTR_PMNC_SW_INCR    = 0x00,
  ARMV7_PERFCTR_IFETCH_MISS   = 0x01,
  ARMV7_PERFCTR_ITLB_MISS     = 0x02,
  ARMV7_PERFCTR_DCACHE_REFILL   = 0x03,
  ARMV7_PERFCTR_DCACHE_ACCESS   = 0x04,
  ARMV7_PERFCTR_DTLB_REFILL   = 0x05,
  ARMV7_PERFCTR_DREAD     = 0x06,
  ARMV7_PERFCTR_DWRITE      = 0x07,
  ARMV7_PERFCTR_EXC_TAKEN     = 0x09,
  ARMV7_PERFCTR_EXC_EXECUTED    = 0x0A,
  ARMV7_PERFCTR_CID_WRITE     = 0x0B,
  /* ARMV7_PERFCTR_PC_WRITE is equivalent to HW_BRANCH_INSTRUCTIONS.
   * It counts:
   *  - all branch instructions,
   *  - instructions that explicitly write the PC,
   *  - exception generating instructions.
   */
  ARMV7_PERFCTR_PC_WRITE      = 0x0C,
  ARMV7_PERFCTR_PC_IMM_BRANCH   = 0x0D,
  ARMV7_PERFCTR_UNALIGNED_ACCESS    = 0x0F,
  ARMV7_PERFCTR_PC_BRANCH_MIS_PRED  = 0x10,
  ARMV7_PERFCTR_CLOCK_CYCLES    = 0x11,
  ARMV7_PERFCTR_PC_BRANCH_MIS_USED  = 0x12,
  ARMV7_PERFCTR_CPU_CYCLES    = 0xFF
};

static phys_addr_t pmu_phys_base;
static struct resource *pmu_resource;
static void *pmu_regs;
static struct task_struct *monitor_task;

static pmu_events_t counters;

static int monitor_thread(void *data);

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

static void setup_events(void)
{
  iowrite32(ARMV7_PERFCTR_IFETCH_MISS, pmu_regs+PMU_PMXEVTYPER0);
  iowrite32(ARMV7_PERFCTR_ITLB_MISS, pmu_regs+PMU_PMXEVTYPER1);
  iowrite32(ARMV7_PERFCTR_PC_BRANCH_MIS_PRED, pmu_regs+PMU_PMXEVTYPER2);
  iowrite32(ARMV7_PERFCTR_PC_IMM_BRANCH, pmu_regs+PMU_PMXEVTYPER3);
}

static int init_ropdetect(void)
{
  phys_addr_t pmu_base;
  unsigned int pmcr;

  if (smp_call_function_single(CPU_TARGET, get_current_debug_regs, &pmu_phys_base, 1) < 0)
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
    goto error_alloc;
  }
  memset(&counters, 0, sizeof(counters));

  pmcr = ioread32(pmu_regs+PMU_PMCR);
  // unlock regs
  iowrite32(0xC5ACCE55, pmu_regs+PMU_PMLAR);
  counters.num_counters = (pmcr >> 11) & 0x1F;
  printk(KERN_DEBUG "Found %d event counters\n", counters.num_counters);
  if (counters.num_counters < 4)
  {
    printk(KERN_WARNING "Not enough event counters! Results will be flawed.\n");
  }
  else if (counters.num_counters >= MAX_EVENT_COUNTERS)
  {
    printk(KERN_ALERT "Too many event counter registers, max supported: %d\n", MAX_EVENT_COUNTERS);
    goto error;
  }
  // clear events
  pmcr |= 0x27; // DP=1, X=0, D=0, C=1, P=1, E=1
  iowrite32(pmcr, pmu_regs+PMU_PMCR);
  // clear overflow
  iowrite32(0xFFFFFFFF, pmu_regs+PMU_PMOVSR);
  iowrite32(0x8000000F, pmu_regs+PMU_PMCNTENSET);

  // create monitor thread
  monitor_task = kthread_create(monitor_thread, NULL, "ropdetect_monitor");
  if (monitor_task == NULL)
  {
    printk(KERN_ALERT "Failed to create monitor thread\n");
    goto error;
  }
  kthread_bind(monitor_task, CPU_MONITOR);
  wake_up_process(monitor_task);

  return 0;
error:
  iowrite32(0x8000000F, pmu_regs+PMU_PMCNTENCLR);
  iounmap(pmu_regs);
error_alloc:
  release_mem_region(pmu_phys_base+PMU_REGS_OFFSET, PMU_REGS_SIZE);
  return -1;
}

static void cleanup_ropdetect(void)
{
  // stop monitor thread
  printk(KERN_DEBUG "Stopping monitor process\n");
  kthread_stop(monitor_task);
  // stop event collection
  iowrite32(0x8000000F, pmu_regs+PMU_PMCNTENCLR);
  release_mem_region(pmu_phys_base+PMU_REGS_OFFSET, PMU_REGS_SIZE);
  iounmap(pmu_regs);
}

static inline void update_counts(void)
{
  int i;
  counters.cycles = ioread32(pmu_regs+PMU_PMCCNTR);
  for (i = 0; i < counters.num_counters; i++)
  {
    counters.events[i] = ioread32(pmu_regs+PMU_PMXEVCNTR0+4*i);
  }
}

static int monitor_thread(void *data)
{
  int cpu;

  cpu = get_cpu();
  if (cpu != CPU_MONITOR)
  {
    printk(KERN_ERR "Running on invalid CPU %d\n", cpu);
    put_cpu();
    return -1;
  }

  // setup event collection
  setup_events();

  while (!kthread_should_stop())
  {
    update_counts();
    if (counters.cycles % 1000 == 0)
    {
      printk(KERN_DEBUG "Cycles: %d, event: %d\n", counters.cycles, counters.events[0]);
    }
  }
  printk(KERN_DEBUG "Monitor process stopped.\n");

  put_cpu();
  return 0;
}

module_init(init_ropdetect);
module_exit(cleanup_ropdetect);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

