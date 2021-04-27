import unittest
import orange

class DomainTestCase(unittest.TestCase):
    def setUp(self):
        self.vars = [orange.Variable.make(x, orange.Variable.Type.Continuous)[0] for x in "abcdefghij"]

    def test_construction(self):
        d = orange.Domain([])
        self.assertEqual(len(d.variables), 0)
        self.assertEqual(len(d.attributes), 0)
        self.assertEqual(d.class_var, None)

        d = orange.Domain([], True)
        self.assertEqual(len(d.variables), 0)
        self.assertEqual(len(d.attributes), 0)
        self.assertEqual(d.class_var, None)

        d = orange.Domain([], False)
        self.assertEqual(len(d.variables), 0)
        self.assertEqual(len(d.attributes), 0)
        self.assertEqual(d.class_var, None)


        d = orange.Domain(self.vars[:1])
        self.assertEqual(d.variables, self.vars[:1])
        self.assertEqual(len(d.attributes), 0)
        self.assertEqual(d.class_var, self.vars[0])

        d = orange.Domain(self.vars[:1], True)
        self.assertEqual(d.variables, self.vars[:1])
        self.assertEqual(len(d.attributes), 0)
        self.assertEqual(d.class_var, self.vars[0])

        d = orange.Domain(self.vars[:1], False)
        self.assertEqual(d.variables, self.vars[:1])
        self.assertEqual(d.attributes, self.vars[:1])
        self.assertEqual(d.class_var, None)


        d = orange.Domain(self.vars)
        self.assertEqual(d.variables, self.vars)
        self.assertEqual(d.attributes, self.vars[:-1])
        self.assertEqual(d.class_var, self.vars[-1])

        d = orange.Domain(self.vars, True)
        self.assertEqual(d.variables, self.vars)
        self.assertEqual(d.attributes, self.vars[:-1])
        self.assertEqual(d.class_var, self.vars[-1])

        d = orange.Domain(self.vars, False)            
        self.assertEqual(d.variables, self.vars)
        self.assertEqual(d.attributes, self.vars)
        self.assertEqual(d.class_var, None)


        d = orange.Domain(self.vars, self.vars[5])
        self.assertEqual(d.variables, self.vars+[self.vars[5]])
        self.assertEqual(d.attributes, self.vars)
        self.assertEqual(d.class_var, self.vars[5])

    def test_construction_parameters(self):
        dold = orange.Domain(self.vars)
        d = orange.Domain(self.vars[:5], dold)
        self.assertEqual(d.variables, self.vars[:5])
        self.assertEqual(d.attributes, self.vars[:4])
        self.assertEqual(d.class_var, self.vars[4])

        d = orange.Domain(["a", "b", "c"], dold)        
        self.assertEqual(d.variables, self.vars[:3])
        self.assertEqual(d.attributes, self.vars[:2])
        self.assertEqual(d.class_var, self.vars[2])

        d = orange.Domain(["a", "b", "c"], True, dold)        
        self.assertEqual(d.variables, self.vars[:3])
        self.assertEqual(d.attributes, self.vars[:2])
        self.assertEqual(d.class_var, self.vars[2])

        d = orange.Domain(["a", "b", "c"], False, dold)        
        self.assertEqual(d.variables, self.vars[:3])
        self.assertEqual(d.attributes, self.vars[:3])
        self.assertEqual(d.class_var, None)
        
        d = orange.Domain(["a", "b", "c"], class_var=False, source=dold)        
        self.assertEqual(d.variables, self.vars[:3])
        self.assertEqual(d.attributes, self.vars[:3])
        self.assertEqual(d.class_var, None)
        
        d = orange.Domain(["a", "b", "c"], source=dold, class_var=False)
        self.assertEqual(d.variables, self.vars[:3])
        self.assertEqual(d.attributes, self.vars[:3])
        self.assertEqual(d.class_var, None)
        
        d = orange.Domain(["a", "b", "c"], source=dold, class_var=True)
        self.assertEqual(d.variables, self.vars[:3])
        self.assertEqual(d.attributes, self.vars[:2])
        self.assertEqual(d.class_var, self.vars[2])

        d = orange.Domain(["a", "b", "c"], source=dold, class_var="d")
        self.assertEqual(d.variables, self.vars[:4])
        self.assertEqual(d.attributes, self.vars[:3])
        self.assertEqual(d.class_var, self.vars[3])

        d = orange.Domain(["a", "b", "c"], source=dold, class_var=self.vars[3])
        self.assertEqual(d.variables, self.vars[:4])
        self.assertEqual(d.attributes, self.vars[:3])
        self.assertEqual(d.class_var, self.vars[3])


    def test_construction_attribute_list(self):
        dold = orange.Domain(self.vars)
        d = orange.Domain([self.vars[1], 0, "c"], source=dold)        
        self.assertEqual(d.variables, [self.vars[1], self.vars[0], self.vars[2]])

        d = orange.Domain([self.vars[1], 0, "c"], source=dold, class_var="d")
        self.assertEqual(d.variables, [self.vars[1], self.vars[0], self.vars[2], self.vars[3]])
        self.assertEqual(d.attributes, [self.vars[1], self.vars[0], self.vars[2]])
        self.assertEqual(d.class_var, self.vars[3])


    def test_construction_domain(self):
        dold = orange.Domain(self.vars)
        d = orange.Domain(dold)
        self.assertIsNot(dold, d)
        self.assertIsNot(dold.variables, d.variables)
        self.assertIsNot(dold.attributes, d.attributes)
        self.assertEqual(dold.variables, d.variables)
        self.assertEqual(dold.attributes, d.attributes)
        self.assertEqual(dold.class_var, d.class_var)

        d = orange.Domain(dold, class_var=False)
        self.assertEqual(dold.variables, d.variables)
        self.assertEqual(dold.variables, d.attributes)
        self.assertIsNone(d.class_var)
            
        d = orange.Domain(dold, class_var=True)
        self.assertEqual(dold.variables, d.variables)
        self.assertEqual(dold.attributes, d.attributes)
        self.assertEqual(dold.class_var, d.class_var)

        d = orange.Domain(dold, class_var="a")
        self.assertEqual(d.variables, self.vars[1:]+[self.vars[0]])
        self.assertEqual(d.attributes, self.vars[1:])
        self.assertEqual(d.class_var, self.vars[0])
            
        d = orange.Domain(dold, class_var=self.vars[0])
        self.assertEqual(d.variables, self.vars[1:]+[self.vars[0]])
        self.assertEqual(d.attributes, self.vars[1:])
        self.assertEqual(d.class_var, self.vars[0])
            
    def test_memory_leaks(self):
        import sys
        f = orange.Domain(self.vars)
        refcount = sys.getrefcount(self.vars[0])
        for i in range(1000):
            f = orange.Domain(self.vars)
        refcount2 = sys.getrefcount(self.vars[0])
        self.assertEqual(refcount, refcount2)

            
    def test_memory_leaks_error(self):
        import sys
        f = orange.Domain(self.vars)
        refcount = sys.getrefcount(self.vars[0])
        for i in range(1000):
            try:
                f = orange.Domain([self.vars[0], "abc"])
            except:
                pass
        refcount2 = sys.getrefcount(self.vars[0])
        self.assertEqual(refcount, refcount2)


    def test_len(self):
        d = orange.Domain(self.vars)
        self.assertEqual(len(d), 10)

        d =orange.Domain(self.vars, class_var=False)
        self.assertEqual(len(d), 10)


    def test_contains(self):
        d = orange.Domain(self.vars[:5])
        
        self.assertIn("a", d)
        self.assertIn(self.vars[0], d)
        self.assertIn(0, d)
        
        self.assertIn("e", d)
        self.assertIn(self.vars[4], d)
        self.assertIn(4, d)

        self.assertNotIn("f", d)
        self.assertNotIn(self.vars[5], d)
        self.assertNotIn(5, d)

        with self.assertRaises(TypeError):
            orange in d

    def test_index(self):
        d = orange.Domain(self.vars)
        
        self.assertEqual(d.index("c"), 2)
        self.assertEqual(d.index(self.vars[2]), 2)
        self.assertEqual(d.index(2), 2)
        
        self.assertEqual(d.index("j"), 9)

        with self.assertRaises(IndexError):
            d.index("x")
        with self.assertRaises(IndexError):
            d.index(orange.ContinuousVariable("x"))
        with self.assertRaises(IndexError):
            d.index(15)
        with self.assertRaises(TypeError):
            d.index[orange]

    def test_item(self):
        d = orange.Domain(self.vars)
        self.assertEqual(d[2], self.vars[2])
        self.assertEqual(d[9], self.vars[9])
        #self.assertEqual(d[-1], d.class_var)
        with self.assertRaises(IndexError):
            d[-2]

        self.assertEqual(d[2], self.vars[2])
        self.assertEqual(d[self.vars[-1].name], self.vars[-1])
        with self.assertRaises(IndexError):
            d["foo"]

        self.assertEqual(d[self.vars[2]], self.vars[2])
        self.assertEqual(d[self.vars[-1]], self.vars[-1])
        with self.assertRaises(IndexError):
            d[orange.ContinuousVariable("foo")]
        with self.assertRaises(IndexError):
            d[orange.ContinuousVariable("a")]
        a = orange.Variable.make("a", orange.Variable.Type.Continuous)[0]
        self.assertIs(a, self.vars[0])
        self.assertEqual(d[a], self.vars[0])

        with self.assertRaises(TypeError):
            d[orange]


    def test_slices(self):
        d = orange.Domain(self.vars)
        self.assertEqual(d[:4], self.vars[:4])
        self.assertEqual(d[2:], self.vars[2:])
        self.assertEqual(d[:], self.vars)
        with self.assertRaises(IndexError):
            d[2:-6]

    def test_repr(self):
        d = orange.Domain([])
        self.assertEqual(repr(d), "<orange.Domain []>")

        d = orange.Domain([self.vars[0]])
        self.assertEqual(repr(d), "<orange.Domain [(no attrs) -> a]>")

        d = orange.Domain([self.vars[0]], class_var=False)
        self.assertEqual(repr(d), "<orange.Domain [a -> (no class)]>")
        
        d = orange.Domain(self.vars, class_var=False)
        self.assertEqual(repr(d), "<orange.Domain [a, b, c, d, e, f, g, h, i, j -> (no class)]>")
        
        d = orange.Domain(self.vars)
        self.assertEqual(repr(d), "<orange.Domain [a, b, c, d, e, f, g, h, i -> j]>")

        d = orange.Domain([orange.ContinuousVariable("A%i" % i) for i in range(1, 101)])
        self.assertEqual(repr(d), "<orange.Domain [%s, ... -> A100]>" % ", ".join("A%i" % i for i in range(1, 21)))

    def test_attrtypes(self):
        d = orange.Domain([])
        self.assertFalse(d.has_continuous_attributes(include_class=True))
        self.assertFalse(d.has_continuous_attributes(include_class=False))
        self.assertFalse(d.has_discrete_attributes(include_class=True))
        self.assertFalse(d.has_discrete_attributes(include_class=False))

        d = orange.Domain([orange.ContinuousVariable("a")])
        self.assertTrue(d.has_continuous_attributes(include_class=True))
        self.assertFalse(d.has_continuous_attributes(include_class=False))
        self.assertFalse(d.has_discrete_attributes(include_class=True))
        self.assertFalse(d.has_discrete_attributes(include_class=False))
        
        d = orange.Domain([orange.DiscreteVariable("a")])
        self.assertFalse(d.has_continuous_attributes(include_class=True))
        self.assertFalse(d.has_continuous_attributes(include_class=False))
        self.assertTrue(d.has_discrete_attributes(include_class=True))
        self.assertFalse(d.has_discrete_attributes(include_class=False))
        
        d = orange.Domain([orange.ContinuousVariable("a"), orange.DiscreteVariable("b")])
        self.assertTrue(d.has_continuous_attributes(include_class=True))
        self.assertTrue(d.has_continuous_attributes(include_class=False))
        self.assertTrue(d.has_discrete_attributes(include_class=True))
        self.assertFalse(d.has_discrete_attributes(include_class=False))

    def test_meta(self):
        d = orange.Domain(self.vars[:5])

        id = orange.newmetaid()
        v5 = self.vars[5]
        
        self.assertFalse(d.hasmeta(v5))
        self.assertFalse(d.hasmeta(id))
        self.assertFalse(d.hasmeta("f"))

        d.addmeta(id, v5)
        self.assertTrue(d.hasmeta(v5))
        self.assertTrue(d.hasmeta(id))
        self.assertTrue(d.hasmeta("f"))

        self.assertEqual(d.metaid(v5), id)
        self.assertEqual(d.metaid(id), id)
        self.assertEqual(d.metaid("f"), id)
        
        self.assertEqual(d.getmeta(v5), v5)
        self.assertEqual(d.getmeta(id), v5)
        self.assertEqual(d.getmeta("f"), v5)

        self.assertFalse(d.is_optional_meta(v5))
        self.assertFalse(d.is_optional_meta(id))
        self.assertFalse(d.is_optional_meta("f"))

        id2 = orange.newmetaid()
        v6 = self.vars[6]
        d.addmeta(id2, v6, 42)

        self.assertTrue(d.hasmeta(v6))

        self.assertEqual(d.getmetas(), {id:v5, id2:v6})
        self.assertEqual(d.getmetas(0), {id:v5})
        self.assertEqual(d.getmetas(42), {id2:v6})

        d.removemeta(v5)
        self.assertFalse(d.hasmeta(v5))

        d.addmetas({id: v5})
        self.assertTrue(d.hasmeta(v5))

        d.removemeta([v5, id2])
        self.assertFalse(d.hasmeta(v5))
        self.assertFalse(d.hasmeta(v6))

        d.addmetas({id: v5, id2: v6})
        self.assertTrue(d.hasmeta(v5))
        self.assertTrue(d.hasmeta(v6))

        d.removemeta(["f", "g"])
        self.assertEqual(d.getmetas(), {})

        with self.assertRaises(IndexError):
            d.metaid("f")
        with self.assertRaises(IndexError):
            d.metaid(id)
        with self.assertRaises(IndexError):
            d.metaid(v5)
        
        with self.assertRaises(IndexError):
            d.getmeta("f")
        with self.assertRaises(IndexError):
            d.getmeta(id)
        with self.assertRaises(IndexError):
            d.getmeta(v5)

        d.addmeta(id2, v6)            
        with self.assertRaises(IndexError):
            d.metaid("f")
        with self.assertRaises(IndexError):
            d.metaid(id)
        with self.assertRaises(IndexError):
            d.metaid(v5)
        
        with self.assertRaises(IndexError):
            d.getmeta("f")
        with self.assertRaises(IndexError):
            d.getmeta(id)
        with self.assertRaises(IndexError):
            d.getmeta(v5)


    def test_features(self):
        d = orange.ExampleTable("iris").domain
        self.assertEqual(d.features, d.attributes)


    def test_aliases(self):
        d = orange.Domain([])

        id = orange.newmetaid()
        d.addmeta(id, orange.ContinuousVariable())
        d.hasmeta(id)

        id = orange.newmetaid()
        d.add_meta(id, orange.ContinuousVariable())
        d.has_meta(id)

    def test_conversion(self):
        d = orange.ExampleTable("iris")
        dom2 = orange.Domain([x[0] for x in [orange.Variable.make("sepal length", orange.ContinuousVariable),
                              orange.Variable.make("petal length", orange.ContinuousVariable),
                              orange.Variable.make("iris", orange.DiscreteVariable)]])
        ex0 = d[0]
        exc = dom2(ex0)
        self.assertEqual(ex0["sepal length"], exc["sepal length"])
        self.assertEqual(ex0["petal length"], exc["petal length"])
        self.assertEqual(ex0["iris"], exc["iris"])
        excb = d.domain(exc)
        self.assertEqual(excb["sepal length"], exc["sepal length"])
        self.assertEqual(excb["petal length"], exc["petal length"])
        self.assertEqual(excb["iris"], exc["iris"])
        self.assertTrue(excb["sepal width"].is_undefined())
        self.assertTrue(excb["petal width"].is_undefined())

        exn = dom2([1, 2, 0])
        self.assertEqual(exn["sepal length"], 1)
        self.assertEqual(exn["petal length"], 2)
        self.assertEqual(exn.getclass(), 0)
    def test_pickle(self):
        import pickle
        d = orange.Domain([])
        s = pickle.dumps(d)
        d2 = pickle.loads(s)
        self.assertEqual(len(d.variables), 0)
        self.assertEqual(len(d.attributes), 0)
        self.assertEqual(d.class_var, None)

        d = orange.Domain(self.vars)
        s = pickle.dumps(d)
        d2 = pickle.loads(s)
        self.assertEqual(d.variables, d2.variables)
        self.assertEqual(d.attributes, d2.attributes)
        self.assertEqual(id(d.variables[0]), id(d2.variables[0]))
        self.assertEqual(d.class_var, d2.class_var)

        d = orange.Domain(self.vars, False)            
        s = pickle.dumps(d)
        d2 = pickle.loads(s)
        self.assertEqual(d.variables, d2.variables)
        self.assertEqual(d.attributes, d2.attributes)
        self.assertEqual(d.class_var, d2.class_var)

        d = orange.Domain(self.vars[:5])
        id1 = orange.newmetaid()
        v5 = self.vars[5]
        d.addmeta(id1, v5)
        id2 = orange.newmetaid()
        v6 = self.vars[6]
        d.addmeta(id2, v6, 42)
        s = pickle.dumps(d)
        d2 = pickle.loads(s)
        self.assertEqual(d.variables, d2.variables)
        self.assertEqual(d.attributes, d2.attributes)
        self.assertEqual(id(d.variables[0]), id(d2.variables[0]))
        self.assertEqual(d.class_var, d2.class_var)
        self.assertTrue(d.hasmeta(v5))
        self.assertTrue(d.hasmeta(id1))
        self.assertTrue(d.hasmeta("f"))
        self.assertTrue(d.hasmeta(v5))
        self.assertTrue(d.hasmeta(id2))
        self.assertTrue(d.hasmeta("g"))
        self.assertEqual(d.getmetas(), {id1:v5, id2:v6})
        self.assertEqual(d.getmetas(0), {id1:v5})
        self.assertEqual(d.getmetas(42), {id2:v6})
        
if __name__ == "__main__":
    unittest.main()