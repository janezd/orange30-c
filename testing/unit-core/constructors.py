import unittest
import orange

class ConstructorKeywordsTestCase(unittest.TestCase):
    def testConstructor(self):
        f = orange.DiscreteVariable("x", values="abc", x=12)
        self.assertEqual(f.name, "x")
        self.assertEqual(f.values, ["a", "b", "c"])
        self.assertEqual(f.x, 12)

        f = orange.DiscreteVariable("x", x=12)
        self.assertEqual(f.name, "x")
        self.assertEqual(f.values, [])
        self.assertEqual(f.x, 12)
        
        f = orange.DiscreteVariable("x", values="abc")
        self.assertEqual(f.name, "x")
        self.assertEqual(f.values, ["a", "b", "c"])

        f = orange.DiscreteVariable("x")        
        self.assertEqual(f.name, "x")
        self.assertEqual(f.values, [])

        f = orange.DiscreteVariable(values="abc", name="x")
        self.assertEqual(f.name, "x")
        self.assertEqual(f.values, ["a", "b", "c"])
        
if __name__ == "__main__":
    unittest.main()