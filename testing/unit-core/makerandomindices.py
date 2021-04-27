import unittest
import orange

class MakeRandomIndices(unittest.TestCase):
        
    def test_MakeRandomIndices2(self):
        d = orange.ExampleTable("iris")

        inds = orange.MakeRandomIndices2(10, p0=5)
        self.assertEqual(sum(inds), 5)

        inds = orange.MakeRandomIndices2(10, p0=0.5)
        self.assertEqual(sum(inds), 5)

        inds = orange.MakeRandomIndices2(10, p0=0)
        self.assertEqual(sum(inds), 10)
        
        inds = orange.MakeRandomIndices2(10, p0=1)
        self.assertEqual(sum(inds), 0)
        
        mr = orange.MakeRandomIndices2(p0=0.3)
        self.assertEqual(sum(mr(10)), 7)
        
        mr.p0 = 0.9
        inds = mr(d)
        self.assertEqual(sum(inds), 15)
        self.assertEqual(len([i for i, fold in enumerate(inds) if fold==0 and d[i].getclass()==0]), 45)

        mr.stratified = mr.Stratification.NotStratified
        inds = mr(d)
        self.assertEqual(sum(inds), 15)
        ## Probably not equal... ;)
        self.assertNotEqual(len([i for i, fold in enumerate(inds) if fold==0 and d[i].getclass()==0]), 45)


    def test_MakeRandomIndicesCV(self):
        d = orange.ExampleTable("iris")

        inds = orange.MakeRandomIndicesCV(100)
        for j in range(10):
            self.assertEqual(len([i for i in inds if i==j]), 10)
        
        inds = orange.MakeRandomIndicesCV(103)
        for j in range(3):
            self.assertEqual(len([i for i in inds if i==j]), 11)
        
        inds = orange.MakeRandomIndicesCV(100, folds=100)
        self.assertEqual(len([i for i in inds if not i]), 1)

        # Check that five of each iris types get into each fold        
        mr = orange.MakeRandomIndicesCV()
        inds = mr(d)
        for j in range(10):
            self.assertEqual(len([i for i in inds if i==j]), 15)
            sel = [d[i].getclass() for i, fold in enumerate(inds) if fold==j]
            for k in range(2):
                self.assertEqual(len([i for i in sel if i==k]), 5)
        
    def test_MakeRandomIndicesN(self):
        d = orange.ExampleTable("iris")

        inds = orange.MakeRandomIndicesN(10, p=[3, 5])
        self.assertEqual(len([i for i in inds if i==0]), 3)
        self.assertEqual(len([i for i in inds if i==1]), 5)
        self.assertEqual(len([i for i in inds if i==2]), 2)
        
        inds = orange.MakeRandomIndicesN(10, p=[.3, .5])
        self.assertEqual(len([i for i in inds if i==0]), 3)
        self.assertEqual(len([i for i in inds if i==1]), 5)
        self.assertEqual(len([i for i in inds if i==2]), 2)

        inds = orange.MakeRandomIndicesN(d, p=[.3, .5])
        self.assertEqual(len([i for i in inds if i==0]), 45)
        self.assertEqual(len([i for i in inds if i==1]), 75)
        self.assertEqual(len([i for i in inds if i==2]), 30)

        self.assertRaises(ValueError, orange.MakeRandomIndicesN, d, p=[300, 12])

        
if __name__ == "__main__":
    unittest.main()        