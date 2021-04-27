import unittest
import orange
import pickle

class VariableTestCase(unittest.TestCase):
    def test_floatvariable(self):
        f = orange.ContinuousVariable()
        self.assertEqual(f.name, "")
        self.assertIsNone(f.source_variable)
        
        self.assertEqual(f.number_of_decimals, 3)
        self.assertEqual(f.adjust_decimals, 2)
        val = orange.PyValue(f, "2.1")
        self.assertEqual(f.number_of_decimals, 1)
        self.assertEqual(f.adjust_decimals, 1)

        self.assertAlmostEqual(float(val), 2.1, 2)

        f = orange.ContinuousVariable("x")
        self.assertEqual(f.name, "x")
        f = orange.ContinuousVariable(name="x")
        self.assertEqual(f.name, "x")
        
        s = pickle.dumps(f)
        f2 = pickle.loads(s)
        self.assertEqual(f.name, f2.name)
        self.assertEqual(id(f), id(f2))

      
    def test_enumvariable(self):
        f = orange.DiscreteVariable()
        self.assertEqual(f.name, "")
        self.assertIsNone(f.source_variable)
        self.assertIsNotNone(f.values)
        self.assertFalse(f.values)
        
        f = orange.DiscreteVariable("x", "abc")
        self.assertEqual(f.name, "x")
        self.assertEqual(f.values, ["a", "b", "c"])
        f = orange.DiscreteVariable(values="abc", name="x")
        self.assertEqual(f.name, "x")
        self.assertEqual(f.values, ["a", "b", "c"])

        v = orange.PyValue(f, "a")
        self.assertEqual(str(v), "a")
        self.assertEqual(int(v), 0)
        
        #v = f.first

    def test_make(self):
        # Creates a new variable
        v1, s = orange.Variable.make("a", orange.VarTypes.Discrete, ["a", "b"])

        # Reuses v1
        v2, s = orange.Variable.make("a", orange.VarTypes.Discrete, ["a"], ["c"])
        self.assertEqual(s, orange.Variable.MakeStatus.MissingValues)
        #self.assertEqual(type(s), orange.Variable.MakeStatus)
        self.assertEqual(v1.values, "abc")
        self.assertIs(v1, v2)

        # Reuses v1
        v3, s = orange.Variable.make("a", orange.VarTypes.Discrete, ["a", "b", "c", "d"])
        self.assertEqual(s, orange.Variable.MakeStatus.MissingValues)
        self.assertEqual(v1.values, "abcd")
        self.assertIs(v1, v3)

        # Reuses v1
        v4, s = orange.Variable.make("a", orange.VarTypes.Discrete, ["a", "b"])
        self.assertEqual(s, orange.Variable.MakeStatus.OK)
        self.assertEqual(v4.values, "abcd")
        self.assertIs(v1, v4)

        # Creates a new one due to incompatibility
        v5, s = orange.Variable.make("a", orange.VarTypes.Discrete, ["b"])
        self.assertEqual(s, orange.Variable.MakeStatus.Incompatible)
        self.assertEqual(v5.values, "b")
        self.assertEqual(v1.values, "abcd")
        self.assertIsNot(v1, v5)

        # Can reuse - the order is not prescribed
        v6, s = orange.Variable.make("a", orange.VarTypes.Discrete, None, ["c", "a"])
        self.assertEqual(s, orange.Variable.MakeStatus.OK)
        self.assertEqual(v6.values, "abcd")
        self.assertIs(v1, v6)

        # Can reuse despite missing and unrecognized values - the order is not prescribed
        v7, s = orange.Variable.make("a", orange.VarTypes.Discrete, None, ["e"])
        self.assertEqual(s, orange.Variable.MakeStatus.NoRecognizedValues)
        self.assertEqual(v7.values, "abcde")
        self.assertIs(v1, v7)

        # Can't reuse due to unrecognized values
        v8, s = orange.Variable.make("a", orange.VarTypes.Discrete, None, ["f"], orange.Variable.MakeStatus.NoRecognizedValues)
        self.assertEqual(s, orange.Variable.MakeStatus.NoRecognizedValues)
        self.assertEqual(v1.values, "abcde")
        self.assertEqual(v8.values, "f")
        self.assertIsNot(v1, v8)

        # No reuse
        v9, s = orange.Variable.make("a", orange.VarTypes.Discrete, ["a", "b", "c", "d", "e"], None, orange.Variable.MakeStatus.OK)
        self.assertIsNot(v1, v9)

    def test_make_continuous(self):
        # Creates a new variable
        v1, s = orange.Variable.make("a", orange.VarTypes.Continuous)

        # Reuses v1
        v2, s = orange.Variable.make("a", orange.VarTypes.Continuous)
        self.assertEqual(s, orange.Variable.MakeStatus.OK)
        self.assertIs(v1, v2)

        # Reuses v1
        v9, s = orange.Variable.make("a", orange.VarTypes.Continuous, create_new_on=orange.Variable.MakeStatus.OK)
        self.assertIsNot(v1, v9)

    def test_make_continuous_old(self):
        # Creates a new variable
        v1 = orange.ContinuousVariable("a123")

        # Reuses v1
        v2, s = orange.Variable.make("a123", orange.VarTypes.Continuous)
        self.assertEqual(s, orange.Variable.MakeStatus.OK)
        self.assertIs(v1, v2)

if __name__ == "__main__":
    unittest.main()