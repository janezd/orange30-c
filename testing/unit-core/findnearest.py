import unittest
import math
import orange

class FindNearestTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def t2est_construction(self):
        d = orange.ExampleTable("iris")
        fnc = orange.FindNearestConstructor()
        fn = fnc(d)
        nearest15a = fn(d[0], 15)

        fn = orange.FindNearestConstructor(d)
        nearest15b = fn(d[0], 15)

        self.assertEqual(nearest15a.checksum(), nearest15b.checksum())
        self.assertEqual(nearest15a[0], d[0])

        fn = orange.FindNearestConstructor(d, include_same=False)
        nearest15b = fn(d[0], 15)
        self.assertNotEqual(nearest15b[0], d[0])

        fn.include_same = True
        id = orange.newmetaid()
        all_sorted = fn(d[0], distanceID=id)
        self.assertEqual(len(all_sorted), 150)
        self.assertEqual(all_sorted[0, id], 0)
        for i in range(149):
            self.assertLessEqual(all_sorted[i, id], all_sorted[i+1, id])

        for ex in d[:10]:
            ex.setclass("?")
        fn.needs_class = True
        all_sorted = fn(d[0], distanceID=id)
        self.assertEqual(len(all_sorted), 150)

    def test_hamming(self):            
        d = orange.ExampleTable("iris")
        id = orange.newmetaid()
        fnc = orange.FindNearestConstructor(distance_constructor=orange.ExamplesDistanceConstructor_Hamming())
        fn = fnc(d)
        nearest15a = fn(d[0], 15)

        fn = orange.FindNearestConstructor(d, distance_constructor=orange.ExamplesDistanceConstructor_Hamming())
        nearest15b = fn(d[0], 15)

        self.assertEqual(nearest15a.checksum(), nearest15b.checksum())

    def test_bruteforce(self):
        self.assertEqual(orange.FindNearestConstructor, orange.FindNearestConstructor_BruteForce)
        self.assertEqual(orange.FindNearest, orange.FindNearest_BruteForce)

        d = orange.ExampleTable("iris")
        fnc = orange.FindNearestConstructor()
        fn = fnc(d)
        nearest15a = fn(d[0], 15)

        fnc = orange.FindNearestConstructor_BruteForce()
        fn = fnc(d)
        nearest15b = fn(d[0], 15)

        self.assertEqual(nearest15a.checksum(), nearest15b.checksum())
                
if __name__ == "__main__":
    unittest.main()