import unittest
import math
import orange

class ExamplesDistanceTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_euclidean(self):
        d = orange.ExampleTable("iris")
        e1 = d[0]
        e2 = d[50]
        euc = orange.ExamplesDistanceConstructor_Euclidean()
        euc.normalize = False
        euc.ignoreClass = True
        eud = euc(d)
        self.assertEqual(eud(e1, e2), math.sqrt(sum((x-y)**2 for x, y in zip(list(e1)[:-1], e2))))
        
        euc.ignoreClass = False
        eud = euc(d)
        self.assertEqual(eud(e1, e2), math.sqrt(sum((x-y)**2 for x, y in zip(e1, e2))))

        euc.normalize = True        
        eud = euc(d)
        eud(e1, e2) # Returns whatever

        d2 = orange.ExampleTable("zoo")
        self.assertRaises(ValueError, eud, d2[0], d2[1])
        self.assertRaises(ValueError, eud, d2[0], d[0])
        eud = euc(d2)
        self.assertEqual(eud(d2[0], d2[0]), 0)
        eud(d2[0], d2[1]) # Returns whatever

    def test_manhattan(self):
        d = orange.ExampleTable("iris")
        e1 = d[0]
        e2 = d[50]
        euc = orange.ExamplesDistanceConstructor_Manhattan()
        euc.normalize = False
        euc.ignoreClass = True
        eud = euc(d)
        self.assertEqual(eud(e1, e2), sum(abs(x-y) for x, y in zip(list(e1)[:-1], e2)))
        
        euc.ignoreClass = False
        eud = euc(d)
        self.assertEqual(eud(e1, e2), sum(abs(x-y) for x, y in zip(e1, e2)))

        euc.normalize = True        
        eud = euc(d)
        eud(e1, e2) # Returns whatever

        d2 = orange.ExampleTable("zoo")
        self.assertRaises(ValueError, eud, d2[0], d2[1])
        self.assertRaises(ValueError, eud, d2[0], d[0])
        eud = euc(d2)
        self.assertEqual(eud(d2[0], d2[0]), 0)
        eud(d2[0], d2[1]) # Returns whatever

    def test_hamming(self):
        d = orange.ExampleTable("iris")
        e1 = d[0]
        e2 = d[50]
        euc = orange.ExamplesDistanceConstructor_Hamming()
        euc.ignoreClass = True
        eud = euc(d)
        self.assertEqual(eud(e1, e2), sum(x!=y for x, y in zip(list(e1)[:-1], e2)))
        
        euc.ignoreClass = False
        eud = euc(d)
        self.assertEqual(eud(e1, e2), sum(x!=y for x, y in zip(e1, e2)))

        d2 = orange.ExampleTable("zoo")
        self.assertEqual(eud(d2[0], d2[1]), sum(x!=y for x, y in zip(d2[0], d2[1])))

    def test_call_construction(self):
        d = orange.ExampleTable("iris")
        e1 = d[0]
        e2 = d[50]
        eud = orange.ExamplesDistanceConstructor_Euclidean(d, ignore_class=True, normalize=False, foo=42)
        self.assertEqual(eud(e1, e2), math.sqrt(sum((x-y)**2 for x, y in zip(list(e1)[:-1], e2))))
        self.assertEqual(eud.foo, 42)

        eud = orange.ExamplesDistanceConstructor_Euclidean(d, ignore_class=False, normalize=False, foo=42)
        self.assertEqual(eud(e1, e2), math.sqrt(sum((x-y)**2 for x, y in zip(list(e1), e2))))
        self.assertEqual(eud.foo, 42)
        
        

if __name__ == "__main__":
    unittest.main()