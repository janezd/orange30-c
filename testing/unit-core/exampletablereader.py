import unittest
import orange

class ExampleTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def tnames(self, domain, varnames):
        self.assertEqual([x.name for x in domain.variables], varnames)
        self.assertEqual([x.name for x in domain.attributes], varnames[:-1])
        self.assertEqual(domain.class_var.name, varnames[-1])
        
    def test_domain(self):
        r = orange.ExampleTableReader("iris.tab")
        f = open("iris.tab")
        vars = f.readline().strip().split("\t")
        f.close()
        r.read_domain()
        d = r.domain
        self.tnames(d, vars)
        self.assertEqual([x.var_type for x in d.attributes], [orange.VarTypes.Continuous]*4)
        self.assertEqual(d.class_var.var_type, orange.Variable.Type.Discrete)
        
        r = orange.ExampleTableReader("zoo.tab")
        f = open("zoo.tab")
        vars = f.readline().strip().split("\t")[1:]
        f.close()
        r.read_domain()
        d = r.domain
        self.tnames(d, vars)
        self.assertEqual([x.var_type for x in d.variables], [orange.VarTypes.Discrete]*len(d.variables))

    def test_data(self):
        r = orange.ExampleTableReader("iris.tab")
        d = r.read()

        r = orange.ExampleTableReader("zoo.tab")
        d = r.read()

    def test_unicode_filename(self):
        r = orange.ExampleTableReader("iris-čšž.tab")
        d = r.read()

    def test_basket(self):
        r = orange.ExampleTableReader("inquisition2.basket", 0)
        d = r.read()
        self.assertEqual(len(d), 10)
        self.assertDictEqual(d[0].get_metas(str),
            {"nobody": 1, "expects": 1, "the": 1, "Spanish": 1, "Inquisition": 5})
        self.assertDictEqual(d[1].get_metas(str),
            {'and': 2.0, 'is': 1.0, 'chief': 1.0, 'our': 1.0, 'surprise': 6.0,
             'fear': 2.0, 'weapon': 1.0}
        )

    def test_pickle(self):
        import pickle
        r = orange.ExampleTableReader("iris.tab")
        self.assertRaises(pickle.PickleError, pickle.dumps, r)
        
if __name__ == "__main__":
    unittest.main()