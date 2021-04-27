import unittest
import pickle
import orange
from math import exp

class SymMatrixTestCase(unittest.TestCase):
    def setUp(self):
        v = 0
        self.m = orange.SymMatrix(5)
        for i in range(5):
            for j in range(i+1):
                self.m[i, j] = v
                v += 1

        self.zerom = orange.SymMatrix(0)

    def test_construction(self):
        m = orange.SymMatrix(5)
        self.assertEqual(m.dim, 5)
        self.assertEqual(m[0, 0], 0)
        self.assertEqual(m[4, 4], 0)
        self.assertEqual(m[0, 4], 0)
        self.assertEqual(m[4, 0], 0)

        m = orange.SymMatrix(5, 42)
        self.assertEqual(m.dim, 5)
        self.assertEqual(m[0, 0], 42)
        self.assertEqual(m[4, 4], 42)
        self.assertEqual(m[0, 4], 42)
        self.assertEqual(m[4, 0], 42)

        self.assertEqual(self.zerom.dim, 0)

        self.assertRaises(ValueError, orange.SymMatrix, -1)


    def test_indexing(self):
        m = orange.SymMatrix(5)
        m[0, 0] = 42
        for i in range(5):
            for j in range(5):
                self.assertEqual(m[i, j], 0 if i or j else 42)

        m[3, 2] = 42
        for i in range(5):
            for j in range(5):
                self.assertEqual(m[i, j], 42 if (i, j) in [(0, 0), (2, 3), (3, 2)] else 0)

        with self.assertRaises(IndexError):
            m[-1, 0]
        with self.assertRaises(IndexError):
            m[5, 0]
        with self.assertRaises(IndexError):
            m[0, -1]
        with self.assertRaises(IndexError):
            m[0, 5]
        with self.assertRaises(IndexError):
            m[-1, -1]

        with self.assertRaises(IndexError):
            self.zerom[0, 0]
        with self.assertRaises(IndexError):
            self.zerom[-1, -1]

    def test_flat(self):
        self.assertEqual(self.m.flat(), list(range(15)))

        self.assertEqual(self.zerom.flat(), [])


    def test_knn(self):
        m = orange.SymMatrix(5, 42)
        m[2, 3] = 5
        m[2, 0] = 6
        m[2, 1] = 7
        self.assertEqual(m.getKNN(2, 3), [3, 0, 1])

        self.assertRaises(IndexError, self.zerom.getKNN, 0, 3)


    def test_pickle(self):
        s = pickle.dumps(self.m)
        m2 = pickle.loads(s)
        self.assertEqual(self.m.flat(), m2.flat())

        s = pickle.dumps(self.zerom)
        m2 = pickle.loads(s)
        self.assertEqual(m2.dim, 0)


    def test_copy(self):
        m = orange.SymMatrix(self.m)
        self.assertEqual(self.m.flat(), m.flat())

        m = orange.SymMatrix(self.zerom)
        self.assertEqual(m.dim, 0)

    def test_linkage(self):
        self.assertRaises(ValueError, self.m.avg_linkage, [[], [1, 2, 3, 4]])
        lk = self.m.avg_linkage([[0], [1, 2], [3, 4]])
        self.assertEqual(lk.flat(), [0, 2, 3.75, 8, 9.5, 12.25])

        lk = self.m.avg_linkage([[0, 1, 2, 3, 4]])
        self.assertEqual(lk.dim, 1)
        self.assertAlmostEqual(lk.flat()[0], 180/25, 4)

        self.assertRaises(IndexError, self.zerom.avg_linkage, [[0]])


    def test_transformations(self):
        m = orange.SymMatrix(self.m)
        m.negate()
        self.assertEqual(m.flat(), [-x for x in range(15)])

        m = orange.SymMatrix(self.m)
        m.invert(orange.SymMatrix.Transformation.Negate)
        self.assertEqual(m.flat(), [-x for x in range(15)])

        m = orange.SymMatrix(self.m)
        m[0,0] = 1
        m.invert()
        for a1, a2 in zip(m.flat(), [1] + [1/x for x in range(1, 15)]):
            self.assertAlmostEqual(a1, a2)

        m = orange.SymMatrix(self.m)
        m[0,0] = 1
        m.invert(m.Transformation.Invert)
        for a1, a2 in zip(m.flat(), [1] + [1/x for x in range(1, 15)]):
            self.assertAlmostEqual(a1, a2)

        m = orange.SymMatrix(self.m)
        m.subtractFromOne()
        self.assertEqual(m.flat(), [1 - x for x in range(15)])

        m = orange.SymMatrix(self.m)
        m.invert(m.Transformation.SubtractFromOne)
        self.assertEqual(m.flat(), [1 - x for x in range(15)])

        m = orange.SymMatrix(self.m)
        m.subtractFromMax()
        self.assertEqual(m.flat(), [14 - x for x in range(15)])

        m = orange.SymMatrix(self.m)
        m.invert(m.Transformation.SubtractFromMax)
        self.assertEqual(m.flat(), [14 - x for x in range(15)])

        self.zerom.invert()
        self.zerom.negate()
        self.zerom.subtractFromOne()
        self.zerom.subtractFromMax()


    def test_normalizations(self):
        m = orange.SymMatrix(self.m)
        m.normalize(m.Normalization.Bounds)
        for a1, a2 in zip(m.flat(), [x/14 for x in range(15)]):
            self.assertAlmostEqual(a1, a2, 4)

        m = orange.SymMatrix(self.m)
        m.normalize(m.Normalization.Sigmoid)
        for a1, a2 in zip(m.flat(), [1/(1+exp(-x)) for x in range(15)]):
            self.assertAlmostEqual(a1, a2, 4)

        m = orange.SymMatrix(5, 42)
        m.normalize(m.Normalization.Bounds)
        self.assertEqual(m.flat(), [0]*15)

        self.zerom.normalize(m.Normalization.Bounds)
        self.zerom.normalize(m.Normalization.Sigmoid)


    def test_getItems(self):
        m = self.m.get_items([1, 3])
        self.assertEqual(m.flat(), [2, 7, 9])

        m = self.m.get_items([1, 3])
        self.assertEqual(m.flat(), [2, 7, 9])

        m = self.m.get_items([1])
        self.assertEqual(m.flat(), [self.m[1, 1]])

        self.assertRaises(IndexError, self.zerom.get_items, [1])

    def test_str(self):
        s = str(self.m.get_items([1, 3]))
        self.assertEqual(s, "((2.000),\n(7.000, 9.000))")

        self.assertEqual(str(self.zerom), "()")

if __name__ == "__main__":
    unittest.main()
