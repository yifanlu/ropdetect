#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "ropdetect.h"

int main(int argc, const char *argv[])
{
  int fp;
  pmu_events_t counters;
  pmu_events_t prev;

  if ((fp = open("/proc/ropdetect", O_RDONLY)) < 0)
  {
    perror("open");
    return -1;
  }

  memset(&prev, 0, sizeof(prev));
  while (read(fp, &counters, sizeof(counters)) > 0)
  {
    if (counters.cycles % 10000 == 0)
    {
      printf("%d, %d, %d, %d, %d\n", counters.cycles, counters.events[0], counters.events[1], counters.events[2], counters.events[3]);
    }
    prev = counters;
  }

  close(fp);
  return 0;
}

