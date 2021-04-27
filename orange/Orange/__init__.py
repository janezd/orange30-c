
# Definitely ugly, but I see no other workaround.
# When, e.g. data.io executes "from orange import ExampleTable"
# orange gets imported again since it is not in sys.modules
# before this entire file is executed
#import sys
#sys.modules["orange"] = orange

from . import data
#from . import data.io
#from . import data.example
#from . import data.sample
#from . import data.value
#from . import data.variable
#from . import data.domain

#from . import graph

#import stat

#from . import probability
#from . import probability.estimate
#from . import probability.distributions
#from . import probability.evd

#from . import classify
#from . import classify.trees
#from . import classify.rules
#from . import classify.lookup
#from . import classify.bayes
#from . import classify.svm
#from . import classify.logreg
#from . import classify.knn
#from . import classify.majority

#from . import regress

#from . import associate

#from . import preprocess
#import preprocess.value
#import preprocess.data

#from . import distances

#from . import wrappers

#from . import featureConstruction
#from . import featureConstruction.univariate
#from . import featureConstruction.functionDecomposition

#from . import cluster
