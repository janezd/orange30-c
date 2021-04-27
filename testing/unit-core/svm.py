import unittest
import pickle
import orange

class SVMTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_learning_cont(self):
        d = orange.ExampleTable("iris")
        bal = orange.SVMLearner()
        ba = bal(d)
        corr = 0
        for e in d:
            if ba(e) == e.getclass():
                corr += 1
        self.assertGreater(corr, 125)

    def test_learning_disc(self):
        d = orange.ExampleTable("zoo")
        bal = orange.SVMLearner(svm_type=orange.SVMLearner.C_SVC)
        ba = bal(d)
        corr = 0
        for e in d:
            if ba(e) == e.getclass():
                corr += 1
        self.assertGreater(corr, 75)

    def test_pickle(self):
        d = orange.ExampleTable("iris")
        ba = orange.SVMLearner(d)
        s = pickle.dumps(ba)
        ba2 = pickle.loads(s)
        for e in d:
            self.assertEqual(ba(e), ba2(e))

        ba1 = orange.SVMLearner()
        s = pickle.dumps(ba1)
        ba2 = pickle.loads(s)


if __name__ == "__main__":
    unittest.main()