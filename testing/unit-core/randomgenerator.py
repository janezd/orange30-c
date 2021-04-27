import unittest
import orange

class RandomGeneratorTestCase(unittest.TestCase):
    def test(self):
        r = orange.RandomGenerator()
        r0 = orange.RandomGenerator(0)
        self.assertEqual(r(), r0())
        r.initseed = 42
        r0.initseed = 42
        r.reset()
        r0.reset()
        self.assertEqual(r(), r0())
        a = r()
        b = r()
        self.assertNotEqual(a, b)

        import pickle
        s = pickle.dumps(r)
        r2 = pickle.loads(s)
        self.assertEqual(r(), r2())
        self.assertEqual(r.uses, 4)
        


if __name__ == "__main__":
    unittest.main()