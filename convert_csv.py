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
with open(output, 'wb') as csvfile:
  writer = csv.writer(csvfile)
  with open(filename, "rb") as f:
    while True:
      raw = f.read(12+4*MAX_EVENT_COUNTERS)
      if not raw:
        break
      data = struct.unpack('iII%dI' % MAX_EVENT_COUNTERS, raw)
      reset, cycles, num_counters, events = data[0], data[1], data[2], data[3:]
      if first == 0:
        reset = 1
        prev_cycles = cycles
        prev_events = events
        first = 1
      if reset == 1:
        start_time = cycles - (prev_cycles - start_time)
      if not start and sum(events) - sum(prev_events) > 0:
        start = 1
      if start and cycles - prev_cycles > 0:
        writer.writerow([str(cycles - start_time), str(events[0] - prev_events[0]), str(events[1] - prev_events[1]), str(events[2] - prev_events[2]), str(events[3] - prev_events[3])])
      prev_cycles = cycles
      prev_events = events