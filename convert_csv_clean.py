import csv
import struct
import sys

MAX_EVENT_COUNTERS = 4

filename = sys.argv[1]
output = sys.argv[2]

with open(output, 'wb') as csvfile:
  writer = csv.writer(csvfile)
  with open(filename, "rb") as f:
    while True:
      raw = f.read(12+4*MAX_EVENT_COUNTERS)
      if not raw:
        break
      data = struct.unpack('iII%dI' % MAX_EVENT_COUNTERS, raw)
      reset, cycles, num_counters, events = data[0], data[1], data[2], data[3:]
      writer.writerow([str(cycles), str(events[0]), str(events[1]), str(events[2]), str(events[3])])
