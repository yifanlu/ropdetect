#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/smp.h>
#include "ropdetect.h"

#define DRIVER_AUTHOR "Yifan Lu <yifanlu@stanford.edu>"
#define DRIVER_DESC   "ROP detection through pref monitor"

#define CPU_TARGET 3
#define CPU_MONITOR 2
#define CACHE_BUFFER_SIZE 0x10000

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

// default collection ids
static int collect_ids[MAX_EVENT_COUNTERS] = {
  ARMV7_PERFCTR_IFETCH_MISS, 
  ARMV7_PERFCTR_ITLB_MISS, 
  ARMV7_PERFCTR_PC_BRANCH_MIS_PRED, 
  ARMV7_PERFCTR_PC_IMM_BRANCH
};

module_param_array(collect_ids, int, NULL, 0);
MODULE_PARM_DESC(collect_ids, "Array of event ids to collect");

// default collection frequency
static int collect_period = 1000;
module_param(collect_period, int, 0);
MODULE_PARM_DESC(collect_period, "Number of cycles per sampling");

static phys_addr_t pmu_phys_base;
static struct resource *pmu_resource;
static void *pmu_regs;
static struct task_struct *monitor_task;

static pmu_events_t *buffer;
static int read_idx;
static int write_idx;
static int num_counters;

static int monitor_thread(void *data);
static int ropdetect_proc_read(struct file *filp, char *buf, size_t count, loff_t *offp);

static const struct file_operations proc_fops = {
  .owner = THIS_MODULE,
  .read = ropdetect_proc_read
};

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

  if ((buffer = kmalloc(CACHE_BUFFER_SIZE * sizeof(*buffer), GFP_KERNEL)) == NULL)
  {
    printk(KERN_ALERT "Unable to allocate buffer for collection\n");
    return -1;
  }
  memset(buffer, 0, CACHE_BUFFER_SIZE * sizeof(*buffer));

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

  pmcr = ioread32(pmu_regs+PMU_PMCR);
  // unlock regs
  iowrite32(0xC5ACCE55, pmu_regs+PMU_PMLAR);
  num_counters = (pmcr >> 11) & 0x1F;
  printk(KERN_DEBUG "Found %d event counters\n", num_counters);
  if (num_counters < 4)
  {
    printk(KERN_WARNING "Not enough event counters! Results will be flawed.\n");
  }
  else if (num_counters > MAX_EVENT_COUNTERS)
  {
    printk(KERN_ALERT "Too many event counter registers, max supported: %d\n", MAX_EVENT_COUNTERS);
    goto error;
  }

  // create proc entry
  if (proc_create("ropdetect", 0, NULL, &proc_fops) == 0)
  {
    printk(KERN_ALERT "Cannot create `ropdetect` proc entry\n");
    goto error;
  }

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
  iounmap(pmu_regs);
error_alloc:
  release_mem_region(pmu_phys_base+PMU_REGS_OFFSET, PMU_REGS_SIZE);
  return -1;
}

static void cleanup_ropdetect(void)
{
  printk(KERN_DEBUG "Stopping monitor process\n");
  kthread_stop(monitor_task);
  release_mem_region(pmu_phys_base+PMU_REGS_OFFSET, PMU_REGS_SIZE);
  iounmap(pmu_regs);
  remove_proc_entry("ropdetect", NULL);
  kfree(buffer);
}

static void setup_events(void)
{
  int pmcr;
  int i;

  // setup pmu
  pmcr = ioread32(pmu_regs+PMU_PMCR);
  pmcr |= 0x27; // DP=1, X=0, D=0, C=1, P=1, E=1
  iowrite32(pmcr, pmu_regs+PMU_PMCR);

  for (i = 0; i < num_counters; i++)
  {
    iowrite32(collect_ids[i], pmu_regs+PMU_PMXEVTYPER0+4*i);
  }

  // start collection
  iowrite32(0x8000000F, pmu_regs+PMU_PMCNTENSET);
}

static void cleanup_events(void)
{
  int pmcr;

  // disable counts
  pmcr = ioread32(pmu_regs+PMU_PMCR);
  pmcr = pmcr & ~1;
  iowrite32(pmcr, pmu_regs+PMU_PMCR);
}

static inline void reset_counts(void)
{
  int pmcr;
  pmu_events_t *counter;

  // reset counts
  pmcr = ioread32(pmu_regs+PMU_PMCR);
  pmcr |= 0x27; // DP=1, X=0, D=0, C=1, P=1, E=1
  iowrite32(pmcr, pmu_regs+PMU_PMCR);

  // clear overflow
  iowrite32(0xFFFFFFFF, pmu_regs+PMU_PMOVSR);

  counter = &buffer[write_idx];
  memset(counter, 0, sizeof(*counter));
  counter->reset = 1;
}

// the follow two functions are not thread safe, but we lose accuracy in exchange 
// for speed
static inline void update_counts(void)
{
  int i;
  pmu_events_t *counter;

  counter = &buffer[write_idx];
  counter->cycles = ioread32(pmu_regs+PMU_PMCCNTR);
  counter->num_counters = num_counters;
  for (i = 0; i < num_counters; i++)
  {
    counter->events[i] = ioread32(pmu_regs+PMU_PMXEVCNTR0+4*i);
  }
  write_idx = (write_idx + 1) % CACHE_BUFFER_SIZE;
  if (read_idx == write_idx) // push read ahead
  {
    read_idx = (write_idx + 1) % CACHE_BUFFER_SIZE;
  }
}

static inline void get_counts(pmu_events_t *counter)
{
  *counter = buffer[read_idx];
  if (buffer[read_idx].reset)
  {
    buffer[read_idx].reset = 0;
  }
  if (read_idx != write_idx-1)
  {
    read_idx = (read_idx + 1) % CACHE_BUFFER_SIZE;
  }
}

static int monitor_thread(void *data)
{
  int cpu;
  unsigned int cycles, prev_cycles;

  cpu = get_cpu();
  if (cpu != CPU_MONITOR)
  {
    printk(KERN_ERR "Running on invalid CPU %d\n", cpu);
    put_cpu();
    return -1;
  }
#ifndef EXCLUSIVE_CPU_ACCESS
  put_cpu();
#endif

  // setup event collection
  setup_events();
  write_idx = 1;
  read_idx = 0;
  reset_counts();

  prev_cycles = 0;
  while (!kthread_should_stop())
  {
    cycles = ioread32(pmu_regs+PMU_PMCCNTR);
    if (cycles - prev_cycles >= collect_period)
    {
      update_counts();
      prev_cycles = cycles;
    }
    if (ioread32(pmu_regs+PMU_PMOVSR) != 0)
    {
      // overflow
      reset_counts();
    }
  }
  printk(KERN_DEBUG "Monitor process stopping.\n");
  cleanup_events();

#ifdef EXCLUSIVE_CPU_ACCESS
  put_cpu();
#endif
  return 0;
}

static int ropdetect_proc_read(struct file *filp, char *buf, size_t count, loff_t *offp)
{
  pmu_events_t counters;

  if (count < sizeof(counters))
  {
    return -EINVAL;
  }

  get_counts(&counters);
  if (copy_to_user(buf, &counters, sizeof(counters)) != 0)
  {
    return -EACCES;
  }

  return sizeof(counters);
}

module_init(init_ropdetect);
module_exit(cleanup_ropdetect);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

