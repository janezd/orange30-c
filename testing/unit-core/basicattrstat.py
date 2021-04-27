import unittest
import orange

class BasicAttrStatTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test(self):
        d = orange.ExampleTable("iris")
        seplen = [float(e[0]) for e in d]

        b = orange.BasicAttrStat(0, d)

        self.assertEqual(b.variable, d.domain[0])
        self.assertAlmostEqual(b.min, min(seplen))
        self.assertAlmostEqual(b.max, max(seplen))
        self.assertAlmostEqual(b.avg, sum(seplen)/len(seplen))

        b = orange.BasicAttrStat("sepal length", d)
        self.assertEqual(b.variable, d.domain[0])
        self.assertAlmostEqual(b.min, min(seplen))
        self.assertAlmostEqual(b.max, max(seplen))
        self.assertAlmostEqual(b.avg, sum(seplen)/len(seplen))

        b = orange.BasicAttrStat(d.domain[0], d)
        self.assertEqual(b.variable, d.domain[0])
        self.assertAlmostEqual(b.min, min(seplen))
        self.assertAlmostEqual(b.max, max(seplen))
        self.assertAlmostEqual(b.avg, sum(seplen)/len(seplen))

        b = orange.BasicAttrStat(d.domain[0])
        for e in d:
            b.add(e[0])
        self.assertEqual(b.variable, d.domain[0])
        self.assertAlmostEqual(b.min, min(seplen))
        self.assertAlmostEqual(b.max, max(seplen))
        self.assertAlmostEqual(b.avg, sum(seplen)/len(seplen))

        b.reset()
        self.assertEqual(b.avg, 0)

    def test_pickle(self):
        d = orange.ExampleTable("iris")
        b = orange.BasicAttrStat(0, d)
        self.assertEqual(b.variable, d.domain[0])

        import pickle
        s = pickle.dumps(b)
        b2 = pickle.loads(s)
        self.assertEqual(b.sum, b2.sum)
        self.assertEqual(b.sum2, b2.sum2)
        self.assertEqual(b.n, b2.n)
        self.assertEqual(b.min, b2.min)
        self.assertEqual(b.max, b2.max)
        self.assertEqual(b.avg, b2.avg)
        self.assertEqual(b.dev, b2.dev)
        self.assertEqual(id(b.variable), id(b2.variable))

if __name__ == "__main__":
    unittest.main()