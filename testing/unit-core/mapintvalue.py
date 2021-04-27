import unittest
import orange
import pickle
import math

class MapIntValueTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test1(self):
        d = orange.ExampleTable("iris")

        mapping = [1, 2, 0]
        mv = orange.MapIntValue(mapping=mapping)
        val = d[0, -1]
        mval = mv(val)
        self.assertEqual(mval, mapping[int(d[0, -1])])
        self.assertEqual(mval.variable, d.domain[-1])

        unknown = orange.PyValue(d.domain[0], "?")
        self.assertEqual(mv(unknown), unknown)

        s = pickle.dumps(mv)
        mv2 = pickle.loads(s)
        self.assertEqual(mv.mapping, mv2.mapping)
        mval2 = mv2(val)
        self.assertEqual(mval, mval2)

if __name__ == "__main__":
#    suite = unittest.TestLoader().loadTestsFromName("__main__.ExampleTableTestCase.test_translate_through_slice")
#    unittest.TextTestRunner(verbosity=2).run(suite)
    unittest.main()