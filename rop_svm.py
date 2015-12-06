import struct
import sys
sys.path.append('libsvm-3.20/python/')
from svmutil import *

MAX_EVENT_COUNTERS = 4
TIME_DELTA = 1000
CLUSTER_POINTS = 2
TRAIN_POINTS = 100000
TEST_POINTS = 1000000

def extractPoint(f):
  point = []
  i = 0
  global is_data
  global first
  global prev_cycles
  global prev_events
  global time
  while i < CLUSTER_POINTS:
    raw = f.read(12+4*MAX_EVENT_COUNTERS)
    if not raw:
      is_data = 0
      break
    data = struct.unpack('iII%dI' % MAX_EVENT_COUNTERS, raw)
    reset, cycles, num_counters, events = data[0], data[1], data[2], data[3:]
    if first == 0 or reset == 1:
      prev_cycles = cycles
      prev_events = events
      first = 1
    #if cycles - prev_cycles > 0:
      #print("cycle:%d prev_cycle:%d diff:%d"%(cycles, prev_cycles, cycles - prev_cycles))
    if cycles - prev_cycles >= TIME_DELTA:
      time += cycles - prev_cycles
      for j in range(MAX_EVENT_COUNTERS):
        point.append(events[j] - prev_events[j])
      prev_events = events
      prev_cycles = cycles
      i += 1
  print point
  return point

train_sets = int(sys.argv[1])
train_set = []
for i in range(0,train_sets):
  train_set.append(sys.argv[2+i])
test_sets = int(sys.argv[2+train_sets])
test_set = []
for i in range(0,test_sets):
  test_set.append(sys.argv[3+train_sets+i])

x, y = [], []
for t in train_set:
  prev_cycles = 0
  prev_events = None
  time = 0
  first = 0
  points_trained = 0
  is_data = 1
  with open(t, "rb") as f:
    while points_trained < TRAIN_POINTS:
      point = extractPoint(f)
      
      points_trained += 1
      if is_data == 0:
        break

      x.append(point)
      y.append(-1)

print len(x)
prob  = svm_problem(y, x)
param = svm_parameter('-t 2 -s 2')
m = svm_train(prob, param)


rop_detected = 0
#t = 1.0
for t in test_set:
  prev_cycles = 0
  prev_events = None
  time = 0
  first = 0
  is_data = 1
  points_tested = 0
  with open(t, "rb") as f:
    while True:
      point = extractPoint(f)
      
      points_tested += 1
      if is_data == 0:
        break

      p_label, p_acc, p_val = svm_predict([-1], [point], m, '-q')
      print p_label
      if p_label[0] == 1:
        rop_detected += 1

      #t+=1.0
      #x.append(point)
      #y.append(-1)
  print rop_detected

#p_label, p_acc, p_val = svm_predict(y, x, m)