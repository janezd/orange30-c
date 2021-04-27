import unittest
import orange

class RandomClassifierTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_discrete(self):
        d = orange.ExampleTable("iris")
        dist = orange.Distribution(d.domain.class_var, d)

        cc = orange.RandomClassifier(d.domain.class_var)
        self.assertEqual(cc.probabilities.variable, cc.class_var)

        cc2 = orange.RandomClassifier(None, dist)
        self.assertEqual(cc2.class_var, d.domain.class_var)
        self.assertEqual(cc2.probabilities.variable, cc2.class_var)
        self.assertEqual(id(cc2.probabilities), id(dist))
        self.assertTrue(all(x==50 for x in cc2.probabilities))
        
        for cl in [cc, cc2]:
            for e in d[0:150:20]:
                anss = set()
                for i in range(5):
                    anss.add(cl(e))
                self.assertEqual(len(anss), 1)
            
            anss = set()
            for e in d:
                anss.add(cl(e))
            self.assertEqual(len(anss), 3)

        for e in d[0:150:20]:
            self.assertTrue(all(x==50 for x in cc2(e, orange.Classifier.GetProbabilities)))
        
        self.assertRaises(TypeError, orange.RandomClassifier, dist)
        self.assertRaises(ValueError, orange.RandomClassifier, None, orange.DiscDistribution())
        self.assertRaises(ValueError, orange.RandomClassifier, d.domain[1], orange.Distribution(d.domain[0]))

        
    def test_continuous(self):
        d = orange.ExampleTable("iris")
        dom2 = orange.Domain(d.domain.attributes)
        d = orange.ExampleTable(dom2, d)
        self.assertEqual(d.domain.class_var.var_type, orange.Variable.Type.Continuous)
        
        dist = orange.Distribution(d.domain.class_var, d)

        cc = orange.RandomClassifier(d.domain.class_var)
        self.assertEqual(cc.class_var, d.domain.class_var)
        self.assertEqual(cc.probabilities.variable, cc.class_var)
        self.assertRaises(ValueError, cc, d[0])

        cc2 = orange.RandomClassifier(None, dist)
        self.assertEqual(cc2.class_var, d.domain.class_var)
        self.assertEqual(cc2.probabilities.variable, cc2.class_var)
        self.assertEqual(id(cc2.probabilities), id(dist))

        for e in d[0:150:20]:
            anss = set()
            for i in range(5):
                anss.add(cc2(e))
            self.assertEqual(len(anss), 1)

        anss = set()
        for e in d:
            anss.add(cc2(e))
        self.assertGreater(len(anss), 10)

    def test_gaussian(self):
        d = orange.ExampleTable("iris")
        var = orange.ContinuousVariable("iq")
        cc2 = orange.RandomClassifier(var, orange.GaussianDistribution(100, 20))
        for e in d[0:150:20]:
            anss = set()
            for i in range(5):
                anss.add(cc2(e))
            self.assertEqual(len(anss), 1)

        anss = set()
        for e in d:
            anss.add(cc2(e))
        self.assertGreater(len(anss), 10)

    def test_pickle(self):
        import pickle
        d = orange.ExampleTable("iris")
        dist = orange.Distribution(d.domain.class_var, d)

        cc = orange.RandomClassifier(d.domain.class_var)
        s = pickle.dumps(cc)
        cc2 = pickle.loads(s)
        self.assertEqual(cc.class_var, cc2.class_var)
        self.assertEqual(cc.probabilities, cc2.probabilities)
        
        cc = orange.RandomClassifier(d.domain.class_var, dist)
        s = pickle.dumps(cc)
        cc2 = pickle.loads(s)
        self.assertEqual(cc.class_var, cc2.class_var)
        self.assertEqual(cc.probabilities, cc2.probabilities)
        
        
    def test_learner(self):
        d = orange.ExampleTable("iris")
        cl = orange.RandomLearner(d)
        
        for e in d[0:150:20]:
            anss = set()
            for i in range(5):
                anss.add(cl(e))
            self.assertEqual(len(anss), 1)
            
        anss = set()
        for e in d:
            anss.add(cl(e))
        self.assertEqual(len(anss), 3)

    def test_learner_fixed(self):
        d = orange.ExampleTable("iris")
        cl = orange.RandomLearner(d, probabilities=[30, 50, 20])
        for e in d[0:150:20]:
            anss = set()
            for i in range(5):
                anss.add(cl(e))
            self.assertEqual(len(anss), 1)

        anss = set()
        for e in d:
            anss.add(cl(e))
        self.assertEqual(len(anss), 3)

    def test_learner_continuous(self):
        d = orange.ExampleTable("iris")
        dom2 = orange.Domain(d.domain.attributes)
        d = orange.ExampleTable(dom2, d)
        dist = orange.Distribution(d.domain.class_var, d)

        cc2 = orange.RandomLearner(d)
        self.assertEqual(cc2.class_var, d.domain.class_var)
        self.assertEqual(cc2.probabilities.variable, cc2.class_var)

        for e in d[0:150:20]:
            anss = set()
            for i in range(5):
                anss.add(cc2(e))
            self.assertEqual(len(anss), 1)

        anss = set()
        for e in d:
            anss.add(cc2(e))
        self.assertGreater(len(anss), 10)


if __name__ == "__main__":
    unittest.main()