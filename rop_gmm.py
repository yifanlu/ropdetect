import numpy as np
from rop_dataextract import *
from sklearn import mixture
import sys

MAX_EVENT_COUNTERS = 4
TIME_DELTA = 1000
CLUSTER_POINTS = 2

g = mixture.GMM(n_components=2, covariance_type='diag')

train_set, test_set = getSetNames(sys.argv)

obs = aggrTimeseries(train_set, -1, CLUSTER_POINTS, MAX_EVENT_COUNTERS, TIME_DELTA)

print len(obs)
g.fit(obs)
print np.round(g.weights_, 2)
print np.round(g.means_, 2)
print np.round(g.covars_, 2)

test = aggrTimeseries(test_set, -1, CLUSTER_POINTS, MAX_EVENT_COUNTERS, TIME_DELTA)

prediction = g.predict(test)
print sum(prediction)
print len(prediction)