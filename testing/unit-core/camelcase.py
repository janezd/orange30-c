import unittest
import orange

class CamelTestCase(unittest.TestCase):
        
    def test_camel(self):
        f = orange.ContinuousVariable()

        # For builtins, it must support both forms
        self.assertEqual(f.numberOfDecimals, 3)
        self.assertEqual(f.number_of_decimals, 3)

        # Both forms must be equivalent - for built-ins,
        # for both setting and getting
        f.numberOfDecimals = 4        
        self.assertEqual(f.numberOfDecimals, 4)
        self.assertEqual(f.number_of_decimals, 4)
        f.number_of_decimals = 5        
        self.assertEqual(f.numberOfDecimals, 5)
        self.assertEqual(f.number_of_decimals, 5)

        # For non-builtins, they must be separate
        f.abc_abc = 5
        self.assertEqual(f.abc_abc, 5)
        with self.assertRaises(AttributeError):
            f.abcAbc
        f.abcAbc = 6
        self.assertEqual(f.abc_abc, 5)
        self.assertEqual(f.abcAbc, 6)
        
        f.defDef = 5
        self.assertEqual(f.defDef, 5)
        with self.assertRaises(AttributeError):
            f.def_def
        f.def_def = 6
        self.assertEqual(f.defDef, 5)
        self.assertEqual(f.def_def, 6)


if __name__ == "__main__":
    unittest.main()        