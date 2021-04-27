import unittest
import orange

class ProbabilityEstimatorTestCase(unittest.TestCase):
    def setUp(self):
        pass


    def test_relative(self):
        d = orange.ExampleTable("iris")
        distr = orange.get_class_distribution(d)
        pe = orange.ProbabilityEstimatorConstructor_relative(distr)
        self.assertEqual(list(pe()), [1/3, 1/3, 1/3])
        self.assertEqual(pe(0), 1/3)
        self.assertEqual(pe(4), 0)
        self.assertEqual(pe(d[0].getclass()), 1/3)
        self.assertEqual(pe("Iris-setosa"), 1/3)
        self.assertRaises(ValueError, pe, "x")
        self.assertRaises(ValueError, pe, "?")
        self.assertRaises(TypeError, pe, orange)
        
        pec = orange.ProbabilityEstimatorConstructor_relative()
        pe = pec(distr)
        self.assertEqual(list(pe()), [1/3, 1/3, 1/3])
        self.assertEqual(list(pe.probabilities), [1/3, 1/3, 1/3])
        
        
    def test_Laplace(self):
        pec = orange.ProbabilityEstimatorConstructor_Laplace()
        pe = pec([22, 5, 2])
        self.assertEqual(list(pe()), [23/32, 6/32, 3/32])
        
       
    def test_m(self):
        pec = orange.ProbabilityEstimatorConstructor_m(m=3)
        pe = pec([22, 5, 2], [1, 1, 1])
        self.assertEqual(list(pe()), [23/32, 6/32, 3/32])
        
        pe = pec([22, 5, 2], [6, 2, 2])
        self.assertEqual(list(pe()), [(22+.6*3)/32, (5+.2*3)/32, (2+.2*3)/32])

        self.assertRaises(ValueError, pec, [22, 5, 2])

    def test_loess(self):
        d = orange.ExampleTable("iris")
        distr = orange.Distribution(0, d)
        pe = orange.ProbabilityEstimatorConstructor_loess(distr)
        mi, ma = min(distr.values()), max(distr.values())
        self.assertTrue(mi < pe(6) < ma)
        
if __name__ == "__main__":
    unittest.main()