import numpy as np
from rop_dataextract import *
from sklearn import mixture
import sys

MAX_EVENT_COUNTERS = 4
TIME_DELTA = 10000
CLUSTER_POINTS = 2
N_COMPONENTS = 2

g = mixture.GMM(n_components=N_COMPONENTS, covariance_type='diag')

train_set, test_set = getSetNames(sys.argv)

print "aggregating data..."
obs = aggrTimeseries(train_set, -1, CLUSTER_POINTS, MAX_EVENT_COUNTERS, TIME_DELTA)
print len(obs)

print "fitting model..."
g.fit(obs)
print np.round(g.weights_, 2)
print np.round(g.means_, 2)
print np.round(g.covars_, 2)

print "aggregating test..."
test = aggrTimeseries(test_set, -1, CLUSTER_POINTS, MAX_EVENT_COUNTERS, TIME_DELTA)

print "testing..."
prediction = g.predict(test)
total=[0,0,0,0,0,0]
for p in prediction:
  total[p] += 1
print total
print len(prediction)
print max(total)/float(sum(total))