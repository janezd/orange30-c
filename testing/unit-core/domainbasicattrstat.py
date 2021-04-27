import unittest
import orange

class DomainBasicAttrStatTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test(self):
        d = orange.ExampleTable("iris")
        seplen = [float(e[0]) for e in d]

        c = orange.DomainBasicAttrStat(d)
        b = c[0]
        self.assertEqual(b.variable, d.domain[0])
        self.assertAlmostEqual(b.min, min(seplen))
        self.assertAlmostEqual(b.max, max(seplen))
        self.assertAlmostEqual(b.avg, sum(seplen)/len(seplen))

        self.assertEqual(id(b), id(c["sepal length"]))
        self.assertEqual(id(b), id(c[d.domain[0]]))

        ll = list(c)
        self.assertEqual(id(b), id(ll[0]))
        self.assertEqual(ll[-1], None)
        self.assertTrue(c.has_class_var)
        
        self.assertEqual(len(c), 5)
        self.assertEqual(len(ll), 5)

        c.purge()        
        self.assertEqual(len(c), 4)

    def test_pickle(self):
        d = orange.ExampleTable("iris")
        seplen = [float(e[0]) for e in d]

        import pickle
        c = orange.DomainBasicAttrStat(d)
        b = c[0]
        s = pickle.dumps(c)
        c2 = pickle.loads(s)
        self.assertEqual(b.variable, d.domain[0])
        self.assertAlmostEqual(b.min, min(seplen))
        self.assertAlmostEqual(b.max, max(seplen))
        self.assertAlmostEqual(b.avg, sum(seplen)/len(seplen))

        self.assertEqual(id(b), id(c["sepal length"]))
        self.assertEqual(id(b), id(c[d.domain[0]]))

        ll = list(c)
        self.assertEqual(id(b), id(ll[0]))
        self.assertEqual(ll[-1], None)
        self.assertTrue(c.has_class_var)
        
        self.assertEqual(len(c), 5)
        self.assertEqual(len(ll), 5)

        c.purge()        
        self.assertEqual(len(c), 4)
        
        
if __name__ == "__main__":
    unittest.main()