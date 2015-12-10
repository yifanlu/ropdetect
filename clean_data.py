import csv
import struct
import sys

MAX_EVENT_COUNTERS = 4

filename = sys.argv[1]
output = sys.argv[2]

prev_cycles = 0
prev_events = [0, 0, 0, 0]
start_time = 0
first = 0
start = 0
offset = 0
cycles_list = []
events_list = []
with open(output, 'wb') as outfile:
  with open(filename, "rb") as f:
    while True:
      raw = f.read(12+4*MAX_EVENT_COUNTERS)
      if not raw:
        break
      data = struct.unpack('iII%dI' % MAX_EVENT_COUNTERS, raw)
      reset, cycles, num_counters, events = data[0], data[1], data[2], data[3:]
      cycles_list.append(cycles)
      events_list.append(events)
  for i in range(1,len(cycles_list)-1):
    # filter outliers
    if cycles_list[i+1] > cycles_list[i] and cycles_list[i-1] > cycles_list[i]:
      cycles_list[i] = cycles_list[i-1]
      events_list[i] = events_list[i-1]
      continue
    if events_list[i+1][0] > events_list[i][0] and events_list[i-1][0] > events_list[i][0]:
      cycles_list[i] = cycles_list[i-1]
      events_list[i] = events_list[i-1]
      continue
    if events_list[i+1][1] > events_list[i][1] and events_list[i-1][1] > events_list[i][1]:
      cycles_list[i] = cycles_list[i-1]
      events_list[i] = events_list[i-1]
      continue
    if events_list[i+1][2] > events_list[i][2] and events_list[i-1][2] > events_list[i][2]:
      cycles_list[i] = cycles_list[i-1]
      events_list[i] = events_list[i-1]
      continue
    if events_list[i+1][3] > events_list[i][3] and events_list[i-1][3] > events_list[i][3]:
      cycles_list[i] = cycles_list[i-1]
      events_list[i] = events_list[i-1]
      continue
    if cycles_list[i+1] < cycles_list[i] and cycles_list[i-1] < cycles_list[i]:
      cycles_list[i] = cycles_list[i-1]
      events_list[i] = events_list[i-1]
      continue
    if events_list[i+1][0] < events_list[i][0] and events_list[i-1][0] < events_list[i][0]:
      cycles_list[i] = cycles_list[i-1]
      events_list[i] = events_list[i-1]
      continue
    if events_list[i+1][1] < events_list[i][1] and events_list[i-1][1] < events_list[i][1]:
      cycles_list[i] = cycles_list[i-1]
      events_list[i] = events_list[i-1]
      continue
    if events_list[i+1][2] < events_list[i][2] and events_list[i-1][2] < events_list[i][2]:
      cycles_list[i] = cycles_list[i-1]
      events_list[i] = events_list[i-1]
      continue
    if events_list[i+1][3] < events_list[i][3] and events_list[i-1][3] < events_list[i][3]:
      cycles_list[i] = cycles_list[i-1]
      events_list[i] = events_list[i-1]
      continue
    if cycles_list[i-1] > cycles_list[i]: # reset
      start_time = cycles_list[i] - (cycles_list[i-1] - start_time)
    if not start and sum(events_list[i]) - sum(events_list[i-1]) > 0:
      start = 1
      offset = cycles_list[i] - start_time
    if start and cycles_list[i] - cycles_list[i-1] > 0:
      outfile.write(struct.pack('iII%dI' % MAX_EVENT_COUNTERS, reset, cycles_list[i] - start_time - offset, num_counters, events_list[i][0] - events_list[i-1][0], events_list[i][1] - events_list[i-1][1], events_list[i][2] - events_list[i-1][2], events_list[i][3] - events_list[i-1][3]))
