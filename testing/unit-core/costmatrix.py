import unittest
import orange
import pickle
import numpy

class CostMatrix(unittest.TestCase):
    def setUp(self):
        self.data = orange.ExampleTable("iris")
        self.class_var = self.data.domain.class_var
        self.matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
        self.num_array = numpy.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]])

    def testConstructor_dim(self):
        m = orange.CostMatrix(3)
        self.assertEqual(m.dimension, 3)
        self.assertIsNone(m.class_var)
        for i in range(3):
            for j in range(3):
                self.assertEqual(m[i, j], i != j)

        m = orange.CostMatrix(3, 42)
        self.assertEqual(m.dimension, 3)
        self.assertIsNone(m.class_var)
        for i in range(3):
            for j in range(3):
                self.assertEqual(m[i, j], 0 if i==j else 42)

        self.assertRaises(ValueError, orange.CostMatrix, 0)
        self.assertRaises(ValueError, orange.CostMatrix, -1)
        self.assertRaises(TypeError, orange.CostMatrix, "wrogn!")


    def testConstructor_var(self):
        m = orange.CostMatrix(self.class_var)
        self.assertEqual(m.dimension, 3)
        self.assertEqual(m.class_var, self.class_var)
        for i in range(3):
            for j in range(3):
                self.assertEqual(m[i, j], i != j)

        m = orange.CostMatrix(self.class_var, 42)
        self.assertEqual(m.dimension, 3)
        self.assertEqual(m.class_var, self.class_var)
        for i in range(3):
            for j in range(3):
                self.assertEqual(m[i, j], 0 if i==j else 42)

        self.assertRaises(ValueError, orange.CostMatrix, self.data.domain[0])


    def testConstructor_dim_mat(self):
        m = orange.CostMatrix(3, self.matrix)
        self.assertEqual(list(m), self.matrix)

        m = orange.CostMatrix(self.class_var, self.matrix)
        self.assertEqual(list(m), self.matrix)

        self.assertRaises(IndexError, orange.CostMatrix, 4, self.matrix)
        self.assertRaises(IndexError, orange.CostMatrix, 2, self.matrix)
        self.assertRaises(IndexError, orange.CostMatrix, 3, [[1], [2], [3]])
        self.assertRaises(IndexError, orange.CostMatrix, 3, [[1, 2, 3, 4], [2], [3]])


    def testConstructor_var_numpy(self):
        m = orange.CostMatrix(3, self.num_array)
        self.assertEqual(list(m), self.num_array.tolist())

        m = orange.CostMatrix(self.class_var, self.num_array)
        self.assertEqual(list(m), self.num_array.tolist())

        self.assertRaises(IndexError, orange.CostMatrix, 4, self.num_array)
        self.assertRaises(IndexError, orange.CostMatrix, 2, self.num_array)
        self.assertRaises(IndexError, orange.CostMatrix, 3, numpy.zeros((3, 1)))


    def testIteration(self):
        m = orange.CostMatrix(self.matrix)
        for e, f in zip(m, self.matrix):
            self.assertEqual(e, f)


    def testLen(self):
        m = orange.CostMatrix(3)
        self.assertEqual(len(m), 3)


    def testIndexing(self):
        m = orange.CostMatrix(self.class_var, self.matrix)
        self.assertEqual(m[0, 0], self.matrix[0][0])
        self.assertEqual(m[0, 2], self.matrix[0][2])
        self.assertEqual(m[2, 0], self.matrix[2][0])
        self.assertEqual(m[2, 2], self.matrix[2][2])
        self.assertEqual(m[0], self.matrix[0])
        self.assertEqual(m[2], self.matrix[2])
        self.assertEqual(m[self.class_var.values[0]], self.matrix[0])
        self.assertEqual(m[self.class_var.values[2]], self.matrix[2])
        self.assertEqual(m[orange.PyValue(self.class_var, 0)], self.matrix[0])
        self.assertEqual(m[orange.PyValue(self.class_var, 2)], self.matrix[2])

        for i, j in ((0, 5), (5, 0), (0, -2), (-2, 0)):
            with self.assertRaises(IndexError):
                m[i, j]
        with self.assertRaises(IndexError):
            m[5]
        with self.assertRaises(IndexError):
            m[-2]
        with self.assertRaises(IndexError):
            m[1, 1, 1]
        with self.assertRaises(ValueError):
            m["foo"]
        with self.assertRaises(TypeError):
            m[orange]
        with self.assertRaises(TypeError):
            m[1:]
        with self.assertRaises(TypeError):
            m[1, 1:3]


    def testGetCost(self):
        m = orange.CostMatrix(self.class_var, self.matrix)
        self.assertEqual(m.getcost(0, 0), self.matrix[0][0])
        self.assertEqual(m.getcost(0, 2), self.matrix[0][2])
        self.assertEqual(m.getcost(2, 0), self.matrix[2][0])
        self.assertEqual(
            m.getcost(self.class_var.values[0], self.class_var.values[2]),
            self.matrix[0][2])

        for i, j in ((0, 5), (5, 0), (0, -2), (-2, 0)):
            self.assertRaises(IndexError, m.getcost, i, j)
        self.assertRaises(ValueError, m.getcost, "foo", "foo")
        self.assertRaises(TypeError, m.getcost, orange, 42)


    def testAssertIndex(self):
        m = orange.CostMatrix(self.class_var, self.matrix)
        m[1, 2] = 42
        self.assertEqual(m[1, 2], 42)

        m[1] = [3.14, 5.55, 1.11]
        self.assertEqual(m[1], [3.14, 5.55, 1.11])


    def testSetCost(self):
        m = orange.CostMatrix(self.class_var, self.matrix)
        m.setcost(0, 0, 42)
        self.assertEqual(m.getcost(0, 0), 42)
        m.setcost(0, 2, 3.14)
        self.assertEqual(m.getcost(0, 2), 3.14)

        m.setcost(self.class_var.values[0], self.class_var.values[2], 2.79)
        self.assertEqual(m[self.class_var.values[0], self.class_var.values[2]], 2.79)
        self.assertRaises(ValueError, m.setcost, "foo", "foo", 42)
        self.assertRaises(TypeError, m.setcost, orange, 42, 42)


    def testPickle(self):
        m = orange.CostMatrix(self.class_var, self.matrix)
        s = pickle.dumps(m)
        m2 = pickle.loads(s)
        self.assertEqual(list(m2), self.matrix)
        self.assertEqual(m2.dimension, m.dimension)
        self.assertEqual(m2.class_var, self.class_var)

        m = orange.CostMatrix(self.matrix)
        s = pickle.dumps(m)
        m2 = pickle.loads(s)
        self.assertEqual(list(m2), self.matrix)
        self.assertEqual(m2.dimension, m.dimension)
        self.assertIsNone(m2.class_var)

if __name__ == "__main__":
    unittest.main()