import unittest
import orange

class DiscDistributionTestCase(unittest.TestCase):
    def setUp(self):
        self.freqs = [4.0, 20.0, 13.0, 8.0, 10.0, 41.0, 5.0]
        s = sum(self.freqs)
        self.rfreqs = [x/s for x in self.freqs]


    def test_fromExamples(self):
        d = orange.ExampleTable("zoo")
        disc = orange.Distribution("type", d)

        disc2 = orange.Distribution(d.domain.classVar, d)
        self.assertEqual(disc, disc2)

        disc3 = orange.Distribution(len(d.domain.attributes), d)        
        self.assertEqual(disc, disc3)

        disc4 = orange.Distribution(-1, d)
        self.assertEqual(disc, disc4)

        disc5 = orange.get_class_distribution(d)        
        self.assertEqual(disc, disc5)

#        disc6 = orange.getClassDistribution(d)        
#        self.assertEqual(disc, disc7)


    def test_construction(self):
        d = orange.ExampleTable("zoo")

        self.assertRaises(TypeError, orange.DiscDistribution, zip(d.domain["type"].values, self.freqs))

        disc = orange.Distribution("type", d)
        disc7 = orange.DiscDistribution(self.freqs)
        self.assertEqual(disc, disc7)

        disc1 = orange.Distribution(d.domain.classVar)
        self.assertTrue(isinstance(disc1, orange.DiscDistribution))
        

    def test_indexing(self):
        d = orange.ExampleTable("zoo")
        indamphibian = int(orange.PyValue(d.domain.classVar, "amphibian"))

        disc = orange.get_class_distribution(d)

        self.assertEqual(len(disc), len(d.domain.classVar.values))
        
        self.assertEqual(disc["mammal"], 41)
        self.assertEqual(disc[indamphibian], 4)
        self.assertEqual(disc[orange.PyValue(d.domain.classVar, "fish")], 13)

        disc["mammal"] = 100
        self.assertEqual(disc[orange.PyValue(d.domain.classVar, "mammal")], 100)

        disc[indamphibian] = 33
        self.assertEqual(disc["amphibian"], 33)

        disc[orange.PyValue(d.domain.classVar, "fish")] = 12
        self.assertEqual(disc["fish"], 12)

        disc = orange.get_class_distribution(d)
        self.assertEqual(list(disc), self.freqs)
        self.assertEqual(disc.values(), self.freqs)
        self.assertEqual(disc.native(), self.freqs)
        self.assertEqual(disc.keys(), d.domain["type"].values)
        self.assertEqual(disc.items(), list(zip(d.domain["type"].values, self.freqs)))


    def test_hash(self):
        d = orange.ExampleTable("zoo")
        disc = orange.Distribution("type", d)

        disc2 = orange.Distribution(d.domain.classVar, d)
        self.assertEqual(hash(disc), hash(disc2))

        disc2[0] += 1
        self.assertNotEqual(hash(disc), hash(disc2))


    def test_add(self):        
        d = orange.ExampleTable("zoo")
        disc = orange.Distribution("type", d)
        
        disc2 = orange.Distribution(d.domain.classVar)
        for ex in d:
            disc2.add(ex[-1])
        self.assertEqual(disc, disc2)
        
        disc3 = orange.Distribution(d.domain.classVar)
        for ex in d:
            disc3.add(int(ex[-1]), 1.0)
        self.assertEqual(disc, disc3)

        disc4 = orange.Distribution(d.domain.classVar)
        for ex in d:
            disc4.add(float(ex[-1]), 1.0)
        self.assertEqual(disc, disc4)

        disc4.add(0, 1e-8)
        self.assertNotEqual(disc4[0], disc[0])


    def test_normalize(self):        
        d = orange.ExampleTable("zoo")
        disc = orange.Distribution("type", d)
        disc.normalize()
        self.assertEqual(disc, self.rfreqs)
        

    def test_modus(self):        
        d = orange.ExampleTable("zoo")
        disc = orange.Distribution("type", d)
        self.assertEqual(str(disc.modus()), "mammal")


    def test_random(self):
        d = orange.ExampleTable("zoo")
        disc = orange.Distribution("type", d)
        ans = set()
        for i in range(1000):
            ans.add(int(disc.random()))
        self.assertEqual(ans, set(range(len(d.domain.classVar.values))))

if __name__ == "__main__":
    unittest.main()