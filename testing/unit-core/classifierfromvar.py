import pickle
import unittest
import orange

class ClassifierFromVarTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_classifier(self):
        d = orange.ExampleTable("zoo")
        c = orange.ClassifierFromVar(d.domain["legs"])
        self.assertEqual(d[0, "legs"], c(d[0]))

        s = pickle.dumps(c)
        c2 = pickle.loads(s)
        self.assertEqual(d[0, "legs"], c2(d[0]))

        c.class_var = c.variable = d.domain["hair"]
        self.assertEqual(d[0, "hair"], c(d[0]))

        c.class_var = c.variable = d.domain[3]
        d2 = d[:,[3, 4, 5]]
        for e, e2 in zip(d, d2):
            self.assertEqual(e[3], c(e))
            self.assertEqual(e[3], c(e2))
            self.assertEqual(e2[0], c(e))
            self.assertEqual(e2[0], c(e2))

    def test_classifierpos(self):
        d = orange.ExampleTable("zoo")
        c = orange.ClassifierFromVarPos(d.domain, d.domain.index("legs"))
        self.assertEqual(d[0, "legs"], c(d[0]))

        s = pickle.dumps(c)
        c2 = pickle.loads(s)
        self.assertEqual(d[0, "legs"], c2(d[0]))

        c.position = d.domain.index("hair")
        c.class_var = d.domain["hair"]
        self.assertEqual(d[0, "hair"], c(d[0]))

        c.position = 3
        c.class_var = d.domain[3]
        d2 = d[:,[3, 4, 5]]
        for e, e2 in zip(d, d2):
            self.assertEqual(e[3], c(e))
            self.assertEqual(e[3], c(e2))
            self.assertEqual(e2[0], c(e))
            self.assertEqual(e2[0], c(e2))




if __name__ == "__main__":
#    suite = unittest.TestLoader().loadTestsFromName("__main__.ExampleTableTestCase.test_translate_through_slice")
#    unittest.TextTestRunner(verbosity=2).run(suite)
    unittest.main()