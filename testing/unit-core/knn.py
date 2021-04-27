import unittest
import orange

class KNNTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_loading(self):
        d = orange.ExampleTable("iris")
        knn_l = orange.kNNLearner(k=5)
        knn = knn_l(d)

        knn2 = orange.kNNLearner(d, k=5)

        self.assertEqual(knn.k, 5)
        self.assertEqual(knn2.k, 5)
        fn = orange.FindNearestConstructor(d, distance_constructor=orange.ExamplesDistanceConstructor_Hamming())
        fn2 = orange.FindNearestConstructor(d, distance_constructor=orange.ExamplesDistanceConstructor_Manhattan())

        cc = 0
        for e in d:
            if knn(e) == e.getclass():
                cc += 1
            self.assertEqual(knn(e), knn2(e))
        self.assertEqual(cc, 150)

    def test_pickle(self):
        d = orange.ExampleTable("iris")
        knn1 = orange.kNNLearner(d, k=15)
        import pickle
        s = pickle.dumps(knn1)
        knn2 = pickle.loads(s)
        for e in d:
            self.assertEqual(knn1(e), knn2(e))

        knnl1 = orange.kNNLearner(k=15)
        s = pickle.dumps(knnl1)
        knnl2 = pickle.loads(s)
        knn1 = knnl1(d)
        knn2 = knnl2(d)
        for e in d:
            self.assertEqual(knn1(e), knn2(e))
        
if __name__ == "__main__":
    unittest.main()