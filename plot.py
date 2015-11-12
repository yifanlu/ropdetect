import plotly.plotly as py
from plotly.graph_objs import *
import struct
import sys

MAX_EVENT_COUNTERS = 4
TIME_DELTA = 100

username = 'yifanlu'
api_key = '633y70mvms'
py.sign_in(username, api_key)

stream_tokens = ['ho6zz8huxr', '96km39iab4', 'rlsnj0bro3', 'ion1mxk3ub']
traces = [None]*4
traces[0] = Scatter(
    name = 'icache refill', 
    x=[],
    y=[],
    stream=dict(
        token=stream_tokens[0],
        maxpoints=100
    )
)
traces[1] = Scatter(
    name = 'tlb refill', 
    x=[],
    y=[],
    stream=dict(
        token=stream_tokens[1],
        maxpoints=100
    )
)
traces[2] = Scatter(
    name = 'branch mispredict', 
    x=[],
    y=[],
    stream=dict(
        token=stream_tokens[2],
        maxpoints=100
    )
)
traces[3] = Scatter(
    name = 'branch taken', 
    x=[],
    y=[],
    stream=dict(
        token=stream_tokens[3],
        maxpoints=100
    )
)
layout = Layout(
    title='ARM Cortex A7 PMU (Core 0)'
)
fig = Figure(data=traces, layout=layout)
print py.plot(fig, filename='RPI Ropdetect Core 0')
streams = [py.Stream(stream_tokens[i]) for i in range(len(stream_tokens))]
for stream in streams:
  stream.open()

if len(sys.argv) > 1:
  filename = sys.argv[1]
else:
  filename = "/proc/ropdetect"

prev_cycles = 0
prev_events = [0]*MAX_EVENT_COUNTERS
time = 0
with open(filename, "rb") as f:
  raw = f.read(12+4*MAX_EVENT_COUNTERS)
  data = struct.unpack('iII%dI' % MAX_EVENT_COUNTERS, raw)
  reset, cycles, num_counters, events = data[0], data[1], data[2], data[3:]
  if reset == 1:
    prev_cycles = 0
    prev_events = [0]*num_counters
  if cycles - prev_cycles >= TIME_DELTA:
    time += cycles - prev_cycles
    for i in range(len(streams)):
      streams[i].write({'x': time, 'y': events[i] - prev_events[i]})
    prev_events = events
  prev_cycles = cycles
