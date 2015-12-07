import numpy as np
from rop_dataextract import *
from sklearn.svm import OneClassSVM
import sys

MAX_EVENT_COUNTERS = 4
TIME_DELTA = 10000
CLUSTER_POINTS = 32
TRAIN_POINTS = 100000
TEST_POINTS = -1

svm = OneClassSVM()

train_set, test_set = getSetNames(sys.argv)

print "aggregating data..."
obs = aggrTimeseries(train_set, TRAIN_POINTS, CLUSTER_POINTS, MAX_EVENT_COUNTERS, TIME_DELTA)
print len(obs)

print "fitting model..."
svm.fit(obs)

print "aggregating test..."
test = aggrTimeseries(test_set, TEST_POINTS, CLUSTER_POINTS, MAX_EVENT_COUNTERS, TIME_DELTA)

print "testing..."
prediction = svm.predict(test)
print sum(prediction)
print len(prediction)