#ifndef ROPDETECT_H
#define ROPDETECT_H

#define MAX_EVENT_COUNTERS 4

struct pmu_events
{
    unsigned int cycles;
    unsigned int num_counters;
    unsigned int events[MAX_EVENT_COUNTERS];
};

typedef struct pmu_events pmu_events_t;

#endif
