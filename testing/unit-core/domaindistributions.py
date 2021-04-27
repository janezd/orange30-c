import unittest
import orange

class DomainDistributionTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_construction(self):
        d = orange.ExampleTable("iris")
        dd = orange.DomainDistributions(d)

        for i in range(4):
            self.assertTrue(isinstance(dd[i], orange.ContDistribution))
            self.assertEqual(id(dd[i]), id(dd[d.domain[i]]))
            self.assertEqual(id(dd[i]), id(dd[d.domain[i].name]))
        self.assertTrue(isinstance(dd[4], orange.DiscDistribution))
        self.assertEqual(id(dd[4]), id(dd[d.domain.classVar]))
        self.assertEqual(id(dd[4]), id(dd["iris"]))
        self.assertEqual(id(dd[4]), id(dd[-1]))

        for i, ddd in enumerate(list(dd)):
            self.assertEqual(id(ddd), id(dd[i]))
            
        dd = orange.DomainDistributions(d, skip_discrete=True)
        for i in range(4):
            self.assertTrue(isinstance(dd[i], orange.ContDistribution))
        self.assertEqual(dd[-1], None)

        dd = orange.DomainDistributions(d, skip_continuous=True)
        for i in range(4):
            self.assertEqual(dd[i], None)
        self.assertTrue(isinstance(dd[-1], orange.DiscDistribution))
        self.assertEqual(list(dd[-1]), [50, 50, 50])

        dd = orange.DomainDistributions(d, skip_continuous=True, skip_discrete=True)
        for i in range(5):
            self.assertEqual(dd[i], None)            

    def test_pickle(self):
        d = orange.ExampleTable("iris")
        dd = orange.DomainDistributions(d)
        import pickle
        s = pickle.dumps(dd)
        dd2 = pickle.loads(s)

        for i in range(4):
            self.assertTrue(isinstance(dd2[i], orange.ContDistribution))
            self.assertEqual(id(dd2[i]), id(dd2[d.domain[i]]))
            self.assertEqual(id(dd2[i]), id(dd2[d.domain[i].name]))
            self.assertEqual(dd[i], dd2[i])
        self.assertTrue(isinstance(dd2[4], orange.DiscDistribution))
        self.assertEqual(id(dd2[4]), id(dd2[d.domain.classVar]))
        self.assertEqual(id(dd2[4]), id(dd2["iris"]))
        self.assertEqual(id(dd2[4]), id(dd2[-1]))

        dd = orange.DomainDistributions(d, skip_discrete=True)
        s = pickle.dumps(dd)
        dd2 = pickle.loads(s)
        for i in range(4):
            self.assertTrue(isinstance(dd2[i], orange.ContDistribution))
        self.assertEqual(dd2[-1], None)

        dd = orange.DomainDistributions(d, skip_continuous=True)
        s = pickle.dumps(dd)
        dd2 = pickle.loads(s)
        for i in range(4):
            self.assertEqual(dd2[i], None)
        self.assertTrue(isinstance(dd2[-1], orange.DiscDistribution))
        self.assertEqual(list(dd2[-1]), [50, 50, 50])



if __name__ == "__main__":
    unittest.main()