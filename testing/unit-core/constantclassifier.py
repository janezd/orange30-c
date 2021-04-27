import unittest
import orange

class ConstantClassifierTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def t2est_discrete(self):
        d = orange.ExampleTable("iris")
        dist = orange.Distribution(d.domain.class_var, d)

        cc = orange.ConstantClassifier(d.domain.class_var)
        self.assertEqual(cc.class_var, d.domain.class_var)
        self.assertEqual(cc.default_distribution.variable, cc.class_var)

        cc2 = orange.ConstantClassifier(dist)
        self.assertEqual(cc2.class_var, d.domain.class_var)
        self.assertEqual(cc2.default_distribution.variable, cc2.class_var)
        self.assertEqual(id(cc2.default_distribution), id(dist))
        self.assertTrue(all(x==50 for x in cc2.default_distribution))
        
        cc3 = orange.ConstantClassifier(d.domain.class_var, None, dist)
        self.assertEqual(cc3.class_var, d.domain.class_var)
        self.assertEqual(cc3.default_distribution.variable, cc3.class_var)
        self.assertEqual(id(cc3.default_distribution), id(dist))
        self.assertTrue(all(x==50 for x in cc3.default_distribution))

        cc4 = orange.ConstantClassifier(d.domain.class_var, "Iris-setosa", dist)
        self.assertEqual(cc4.class_var, d.domain.class_var)
        self.assertEqual(cc4.default_distribution.variable, cc3.class_var)
        self.assertEqual(id(cc4.default_distribution), id(dist))
        self.assertTrue(all(x==50 for x in cc4.default_distribution))

        for cl in [cc, cc2, cc3]:
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
            anss = set()
            for i in range(5):
                self.assertEqual(cc4(e), "Iris-setosa")

        for cl in [cc2, cc3, cc4]:
            for e in d[0:150:20]:
                self.assertTrue(all(x==50 for x in cl(e, orange.Classifier.GetProbabilities)))
        
        self.assertRaises(TypeError, orange.ConstantClassifier, d.domain.class_var, dist)
        self.assertRaises(ValueError, orange.ConstantClassifier, None, "?", orange.DiscDistribution())
        self.assertRaises(ValueError, orange.ConstantClassifier, d.domain[1], "?", orange.Distribution(d.domain[0]))

        cc4.default_distribution = [50, 50, 50]
        self.assertEqual(list(cc4.default_distribution), [50, 50, 50])

        cc5 = orange.ConstantClassifier(d.domain.class_var, "Iris-setosa", [50, 50, 50])
        self.assertEqual(list(cc5.default_distribution), [50, 50, 50])

    def t2est_continuous(self):
        d = orange.ExampleTable("iris")
        dom2 = orange.Domain(d.domain.attributes)
        d = orange.ExampleTable(dom2, d)
        self.assertEqual(d.domain.class_var.var_type, orange.Variable.Continuous)
        
        dist = orange.Distribution(d.domain.class_var, d)

        cc = orange.ConstantClassifier(d.domain.class_var)
        self.assertEqual(cc.class_var, d.domain.class_var)
        self.assertEqual(cc.default_distribution.variable, cc.class_var)

        cc2 = orange.ConstantClassifier(dist)
        self.assertEqual(cc2.class_var, d.domain.class_var)
        self.assertEqual(cc2.default_distribution.variable, cc2.class_var)
        self.assertEqual(id(cc2.default_distribution), id(dist))
        
        cc3 = orange.ConstantClassifier(d.domain.class_var, None, dist)
        self.assertEqual(cc3.class_var, d.domain.class_var)
        self.assertEqual(cc3.default_distribution.variable, cc3.class_var)
        self.assertEqual(id(cc3.default_distribution), id(dist))

        cc4 = orange.ConstantClassifier(d.domain.class_var, 5, dist)
        self.assertEqual(cc4.class_var, d.domain.class_var)
        self.assertEqual(cc4.default_distribution.variable, cc3.class_var)
        self.assertEqual(id(cc4.default_distribution), id(dist))

        for cl in [cc2, cc3]:
            for e in d:
                self.assertEqual(cl(e), dist.average())

        for e in d:
            self.assertEqual(cc4(e), 5)

        self.assertRaises(TypeError, orange.ConstantClassifier, d.domain.class_var, dist)
        self.assertRaises(ValueError, orange.ConstantClassifier, None, "?", orange.DiscDistribution())
        self.assertRaises(ValueError, orange.ConstantClassifier, d.domain[1], "?", orange.Distribution(d.domain[0]))

        cc4.default_distribution = [50, 50, 50]
        self.assertEqual(list(cc4.default_distribution), [50, 50, 50])


    def test_pickle(self):
        import pickle
        d = orange.ExampleTable("iris")
        dist = orange.Distribution(d.domain[0], d)
        cc = orange.ConstantClassifier(dist)
        s = pickle.dumps(cc)
        cc2 = pickle.loads(s)
        self.assertEqual(cc.class_var, cc2.class_var)
        self.assertEqual(cc.default_val, cc2.default_val)
        self.assertEqual(cc.default_distribution, cc2.default_distribution)

        cc.default_val = 42        
        s = pickle.dumps(cc)
        cc2 = pickle.loads(s)
        self.assertEqual(cc.default_val, cc2.default_val)

        dist = orange.Distribution(d.domain.class_var, d)
        cc = orange.ConstantClassifier(dist)
        s = pickle.dumps(cc)
        cc2 = pickle.loads(s)
        self.assertEqual(cc.class_var, cc2.class_var)
        self.assertEqual(cc.default_val, cc2.default_val)
        self.assertEqual(cc.default_distribution, cc2.default_distribution)

        cc.default_val = 1       
        s = pickle.dumps(cc)
        cc2 = pickle.loads(s)
        self.assertEqual(cc.default_val, cc2.default_val)

if __name__ == "__main__":
    unittest.main()