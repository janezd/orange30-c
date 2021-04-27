import unittest
import orange

class MajorityTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_mammal(self):
        d = orange.ExampleTable("zoo")
        cc = orange.MajorityLearner(d)
        cd = orange.get_class_distribution(d)
        cd.normalize()
        
        for e in d:
            self.assertEqual(cc(e), "mammal")
            self.assertEqual(cc(e, orange.Classifier.GetProbabilities), cd)
        
    def test_equal(self):
        d = orange.ExampleTable("iris")
        cc = orange.MajorityLearner(d)
        
        for e in d[0:150:20]:
            anss = set()
            for i in range(5):
                anss.add(cc(e))
            self.assertEqual(len(anss), 1)
            
        anss = set()
        for e in d:
            anss.add(cc(e))
        self.assertEqual(len(anss), 3)
        for e in d[0:150:20]:
            self.assertTrue(all(x==1/3 for x in cc(e, orange.Classifier.GetProbabilities)))

        import pickle
        s = pickle.dumps(cc)
        cc2 = pickle.loads(s)
        for e in d:
            self.assertEqual(cc(e), cc2(e))
        
            
if __name__ == "__main__":
    unittest.main()