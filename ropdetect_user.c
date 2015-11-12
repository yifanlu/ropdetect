#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "ropdetect.h"

// Number of cycles for each event probe
#define TIME_DELTA 100

int main(int argc, const char *argv[])
{
  const char *filename;
  int fp;
  pmu_events_t counters;
  pmu_events_t prev;

  if (argc > 1)
  {
    filename = argv[1];
  }
  else
  {
    filename = "/proc/ropdetect";
  }

  if ((fp = open(filename, O_RDONLY)) < 0)
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

