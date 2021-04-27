import unittest
import pickle
import orange

class BayesTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_learning_cont(self):
        d = orange.ExampleTable("iris")
        bal = orange.BayesLearner()
        ba = bal(d)
        corr = 0
        for e in d:
            if ba(e) == e.getclass():
                corr += 1
        self.assertGreater(corr, 125)

    def test_learning_disc(self):
        d = orange.ExampleTable("zoo")
        bal = orange.BayesLearner()
        ba = bal(d)
        corr = 0
        for e in d:
            if ba(e) == e.getclass():
                corr += 1
        self.assertGreater(corr, 75)

    def test_pickle(self):
        d = orange.ExampleTable("iris")
        ba = orange.BayesLearner(d)
        s = pickle.dumps(ba)
        ba2 = pickle.loads(s)
        for e in d:
            self.assertEqual(ba(e), ba2(e))

        ba1 = orange.BayesLearner()
        s = pickle.dumps(ba1)
        ba2 = pickle.loads(s)

    def test_named_const(self):
        ba = orange.BayesLearner()
        self.assertEqual(ba.loess_distribution_method, orange.BayesLearner.DistributionMethod.Fixed)
        s = pickle.dumps(ba)
        ba2 = pickle.loads(s)
        self.assertEqual(ba2.loess_distribution_method, orange.BayesLearner.DistributionMethod.Fixed)
        ba.loess_distribution_method = orange.BayesLearner.DistributionMethod.Uniform
        s = pickle.dumps(ba)
        ba2 = pickle.loads(s)
        self.assertEqual(ba2.loess_distribution_method, orange.BayesLearner.DistributionMethod.Uniform)

    def test_subclass(self):
        class MyBayesClassifier(orange.BayesClassifier):
            def foo(self):
                return 42

        class MyBayesLearner(orange.BayesLearner):
            __classifier__ = MyBayesClassifier
            
        d = orange.ExampleTable("iris")
        ba = MyBayesLearner(d)
        self.assertIsInstance(ba, MyBayesClassifier)
        self.assertEqual(ba.foo(), 42)

        class MyBayesClassifier:
            def foo(self):
                return 42

        MyBayesLearner.__classifier__ = MyBayesClassifier
        self.assertRaises(TypeError, MyBayesLearner, d)
        
if __name__ == "__main__":
    unittest.main()