import unittest
import orange

class ContDistributionTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_construction(self):
        d = orange.ExampleTable("iris")
        dist = orange.Distribution("sepal length", d)

        import collections
        dictdist = collections.defaultdict(float)
        for e in d:
            dictdist[float(e["sepal length"])] += 1

        self.assertEqual(dist, dictdist)

        dist2 = orange.ContDistribution(dictdist)
        self.assertEqual(dist, dist2)

    def test_indexing(self):
        d = orange.ExampleTable("iris")
        dist = orange.Distribution("sepal length", d)

        import collections
        dictdist = collections.defaultdict(float)
        for e in d:
            dictdist[float(e["sepal length"])] += 1

        self.assertEqual(dist[4.4], 3)

        self.assertEqual(dist[orange.PyValue(d.domain[0], 4.4)], 3)

        self.assertEqual(len(dist), len(dictdist))

        self.assertEqual(set(dist.keys()), set(dictdist.keys()))   
        self.assertEqual(set(dist.values()), set(dictdist.values()))
        self.assertEqual(set(dist.items()), set(dictdist.items()))
        self.assertEqual(dist.native(), dictdist)

        dist[42] = 2011
        self.assertEqual(dist[42], 2011)
        dist[4.4] = 1971
        self.assertEqual(dist[4.4], 1971)
        


    def test_hash(self):
        d = orange.ExampleTable("iris")
        dist = orange.Distribution("sepal length", d)
        dist2 = orange.Distribution(dist)

        self.assertEqual(hash(dist), hash(dist2))

        dist2[4.4] += 1
        self.assertNotEqual(hash(dist), hash(dist2))

        dist2[4.4] -= 1
        self.assertEqual(hash(dist), hash(dist2))

        dist2[42] = 2011
        self.assertNotEqual(hash(dist), hash(dist2))

    def test_add(self):        
        d = orange.ExampleTable("iris")
        dist = orange.Distribution(0, d)
        
        dist2 = orange.Distribution(d.domain[0])
        for ex in d:
            dist2.add(ex[0])
        self.assertEqual(dist, dist2)
        
        dist3 = orange.ContDistribution()
        for ex in d:
            dist3.add(ex[0])
        self.assertEqual(dist, dist3)

        dist4 = orange.ContDistribution()
        for ex in d:
            dist4.add(ex[0], 1.0)
        self.assertEqual(dist, dist4)

        dist4.add(0, 1e-8)
        self.assertNotEqual(dist4[0], dist[0])


    def test_normalize(self):        
        d = orange.ExampleTable("iris")
        dist = orange.Distribution(0, d)
        dist2 = orange.Distribution(dist)
        dist.normalize()
        self.assertTrue(all(x==y/150 for x, y in zip(dist.values(), dist2.values())))


    def test_modus(self):        
        d = orange.ExampleTable("iris")
        dist = orange.Distribution(0, d)
        self.assertEqual(dist.modus(), 5.0)


    def test_random(self):
        d = orange.ExampleTable("iris")
        dist = orange.Distribution(0, d)
        self.assertTrue(dist.random() in dist.keys())

        rands = set()
        for i in range(100):
            rands.add(float(dist.random()))
        self.assertTrue(len(rands) > 1)

    def test_stat(self):
        d = orange.ExampleTable("iris")
        dist = orange.Distribution(0, d)
        self.assertAlmostEqual(dist.average(), 5.843333333)
        self.assertAlmostEqual(dist.dev(), 0.8253012, 4)

        self.assertEqual(dist.percentile(50), 5.8)
        self.assertRaises(ValueError, dist.percentile, -5)
        self.assertRaises(ValueError, dist.percentile, 105)

        self.assertEqual(dist.density(5.0), 10)
        self.assertEqual(dist.density(4.95), 8)
        
if __name__ == "__main__":
    unittest.main()