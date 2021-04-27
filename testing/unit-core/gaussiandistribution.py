import unittest
import orange

class GaussianDistributionTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_construction(self):
        d = orange.ExampleTable("iris")
        dist = orange.Distribution("sepal length", d)

        g = orange.GaussianDistribution(dist)
        self.assertAlmostEqual(g.average(), dist.average())
        self.assertAlmostEqual(g.modus(), dist.average())
        self.assertAlmostEqual(g.var(), dist.var())
        self.assertAlmostEqual(g.dev(), dist.dev())

        g2 = orange.GaussianDistribution(dist.average(), dist.dev())
        self.assertEqual(g, g2)
"""        
    def test_construction2(self):
        g0 = orange.GaussianDistribution()
        g1 = orange.GaussianDistribution(0)
        g2 = orange.GaussianDistribution(0, 1)
        self.assertEqual(g0, g1)
        self.assertEqual(g0, g2)
        
    def test_random(self):
        g = orange.GaussianDistribution()
        rands = set()
        for i in range(10):
            rands.add(float(g.random()))
        self.assertTrue(len(rands) > 1)

    def test_pickle(self):
        import pickle
        g = orange.GaussianDistribution(42, 3)
        s = pickle.dumps(g)
        g2 = pickle.loads(s)
        self.assertEqual(g, g2)

        g.variable = orange.ContinuousVariable("x")
        s = pickle.dumps(g)
        g2 = pickle.loads(s)
        self.assertEqual(g, g2)
        self.assertEqual(g.variable, g2.variable)
"""     
        

if __name__ == "__main__":
    unittest.main()