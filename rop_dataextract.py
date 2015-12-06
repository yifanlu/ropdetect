import struct

def extractPoint(f, cluster_points, max_event_counters, time_delta, state):
  point = []
  i = 0
  is_data = 1

  while i < cluster_points:
    raw = f.read(12+4*max_event_counters)
    if not raw:
      is_data = 0
      break
    data = struct.unpack('iII%dI' % max_event_counters, raw)
    reset, cycles, num_counters, events = data[0], data[1], data[2], data[3:]
    if state[0] == 0 or reset == 1:
      state[1] = cycles
      state[2] = events
      state[0] = 1
    #if cycles - prev_cycles > 0:
      #print("cycle:%d prev_cycle:%d diff:%d"%(cycles, prev_cycles, cycles - prev_cycles))
    if cycles - state[1] >= time_delta:
      state[3] += cycles - state[1]
      for j in range(max_event_counters):
        point.append(events[j] - state[2][j])
      state[2] = events
      state[1] = cycles
      i += 1
  return [point, is_data]

def getSetNames(argv):
  train_sets = int(argv[1])
  train_set = []
  for i in range(0,train_sets):
    train_set.append(argv[2+i])
  test_sets = int(argv[2+train_sets])
  test_set = []
  for i in range(0,test_sets):
    test_set.append(argv[3+train_sets+i])
  return [train_set, test_set]

def aggrTimeseries(set, points, cluster_points, max_event_counters, time_delta):
  obs = []
  for t in set:
    state = [0,0,None,0]
    points_gathered = 0
    with open(t, "rb") as f:
      while points_gathered < points or points == -1:
        point, is_data = extractPoint(f, cluster_points, max_event_counters, time_delta, state)
        points_gathered += 1

        if is_data == 0:
          break

        obs.append(point)

  return obs