import unittest
import orange

class ExampleTestCase(unittest.TestCase):
    def setUp(self):
        self.contvars = [orange.ContinuousVariable(x) for x in "abcde"]
        self.discvars = [orange.DiscreteVariable(x, values=["ana", "berta", "cilka"]) for x in "ABCDE"]
        self.yvar = [orange.DiscreteVariable("y", values="01")]
        self.contdomain = orange.Domain(self.contvars, self.yvar)
        self.discdomain = orange.Domain(self.discvars, self.yvar)
        self.allvars = self.contvars+self.discvars+[self.yvar]
        self.domain = orange.Domain(self.contvars+self.discvars, self.yvar)

    def test_construction(self):
        e = orange.Example(self.contdomain)
        for val in e:
            self.assertTrue(val.is_undefined())

        e = orange.Example(self.contdomain, "01234")
        for i, val in enumerate(e):
            self.assertEqual(i, val)

        vals = ["ana"]*3+["berta", "cilka"]
        e = orange.Example(self.discdomain, vals)
        for v, ve in zip(vals, e):
            self.assertEqual(v, ve)

        e = orange.Example(self.discdomain, [0, 0, 0, 1, 2])
        for v, ve in zip(vals, e):
            self.assertEqual(v, ve)

        with self.assertRaises(TypeError):
            orange.Example(self.contdomain, 3.14)

        with self.assertRaises(ValueError):
            orange.Example(self.contdomain, "0123")

        with self.assertRaises(ValueError):
            orange.Example(self.contdomain, "012345")

        with self.assertRaises(ValueError):
            orange.Example(self.contdomain, "abcde")

        with self.assertRaises(ValueError):
            orange.Example(self.discdomain, "abcde")

        with self.assertRaises(ValueError):
            orange.Example(self.discdomain, "00110")

    def test_indexing(self):
        e = orange.Example(self.contdomain)

        self.assertEqual(len(e), 5)        

        for i in range(5):
            e[i] = i
        for i in range(5):
            self.assertEqual(e[i], i)
        self.assertEqual(e.getclass(), 4)
        e.setclass(42)
        self.assertEqual(e.getclass(), 42)

        vals = ["ana"]*3+["berta", "cilka"]
        e = orange.Example(self.discdomain)
        for i in range(5):
            e[i] = vals[i]
        for i in range(5):
            self.assertEqual(e[i], vals[i])

        e.setclass("ana")
        self.assertEqual(e.getclass(), "ana")
        e.setclass("cilka")

        self.assertEqual(e[0], "ana")
        self.assertEqual(e[3], "berta")
        self.assertEqual(e[4], "cilka")
        self.assertEqual(e[-1], "cilka")
        self.assertEqual(e["A"], "ana")
        self.assertEqual(e["D"], "berta")
        self.assertEqual(e["E"], "cilka")
        self.assertEqual(e[self.discvars[0]], "ana")
        self.assertEqual(e[self.discvars[3]], "berta")
        self.assertEqual(e[self.discvars[4]], "cilka")
        self.assertEqual(e[self.domain["A"]], "ana")
        self.assertEqual(e[self.domain["D"]], "berta")
        self.assertEqual(e[self.domain["E"]], "cilka")
        

    def test_hash(self):
        e = orange.Example(self.contdomain, "01234")
        self.assertTrue(isinstance(hash(e), int))
        self.assertTrue(isinstance(e.id, int))

    def test_native(self):
        vals = [0, 1, 2, 3, 4, "ana", "ana", "ana", "berta", "cilka"]
        e = orange.Example(self.domain, vals)
        self.assertEqual(e.native(0), vals)
        self.assertEqual(e.native(1), [orange.PyValue(var, val) for var, val in zip(self.allvars, vals)])
        self.assertEqual(e.native(), [orange.PyValue(var, val) for var, val in zip(self.allvars, vals)])

    def test_meta_free(self):
        e = orange.Example(self.contdomain)
        e[0] = 3.14
        mid1 = orange.newmetaid()
        e[mid1] = 2.79
        self.assertEqual(e[0], 3.14)
        self.assertEqual(e[mid1], 2.79)

        mid2 = orange.newmetaid()
        nf = orange.ContinuousVariable("m2")
        self.contdomain.addmeta(mid2, nf)
        e[nf] = 6.28
        self.assertEqual(e[nf], 6.28)
        self.assertEqual(e[mid2], 6.28)
        self.assertEqual(e["m2"], 6.28)

        mid3 = orange.newmetaid()
        sf = orange.StringVariable("m3")
        self.contdomain.addmeta(mid3, sf)
        e["m3"] = "pixies"
        self.assertEqual(e[sf], "pixies")
        self.assertEqual(e[mid3], "pixies")
        self.assertEqual(e["m3"], "pixies")

        self.assertEqual(set(e.get_metas().values()), set([2.79, 6.28, "pixies"]))
        
        del e[mid1]
        with self.assertRaises(KeyError):
            e[mid1]
        self.assertEqual(set(e.get_metas().values()), set([6.28, "pixies"]))
        
        del e[mid3]
        with self.assertRaises(KeyError):
            e[sf]
        self.assertEqual(set(e.get_metas().values()), set([6.28]))

        del e[mid2]
        with self.assertRaises(KeyError):
            e["m2"]
        self.assertEqual(e.get_metas(), {})

        with self.assertRaises(TypeError):
            e[mid1] = "3.15"
        with self.assertRaises(TypeError):
            e[mid3] = 3.15

        with self.assertRaises(TypeError):
            e.set_meta(mid1, "3.15")
        with self.assertRaises(TypeError):
            e.set_meta(mid3 = 3.15)

        e.set_meta(mid1, 3.15)
        e.set_meta("m2", 3.16)
        e.set_meta(sf, "3.17")

        self.assertTrue(e.has_meta(mid1))
        self.assertTrue(e.has_meta(nf))
        self.assertTrue(e.has_meta(sf))

        e.remove_meta(sf)
        self.assertTrue(e.has_meta(mid1))
        self.assertTrue(e.has_meta(nf))
        self.assertFalse(e.has_meta(sf))

        e.remove_meta(mid1)
        self.assertFalse(e.has_meta(mid1))
        self.assertTrue(e.has_meta(nf))
        self.assertFalse(e.has_meta(sf))

        e.remove_meta("m2")        
        self.assertFalse(e.has_meta(mid1))
        self.assertFalse(e.has_meta(nf))
        self.assertFalse(e.has_meta(sf))

        with self.assertRaises(KeyError):
            e.remove_meta(mid1)
        with self.assertRaises(KeyError):
            e.remove_meta("m2")
        with self.assertRaises(KeyError):
            e.remove_meta(sf)
        with self.assertRaises(KeyError):
            del e[mid1]
        with self.assertRaises(KeyError):
            del e["m2"]
        with self.assertRaises(KeyError):
            del e[sf]

        
    def test_meta_direct(self):
        e = orange.ExampleTableReader("iris.tab", 3).read()[0]
        d = e.domain
        e[0] = 3.14
        mid1 = orange.newmetaid()
        e[mid1] = 2.79
        self.assertEqual(e[0], 3.14)
        self.assertEqual(e[mid1], 2.79)

        mid2 = orange.newmetaid()
        nf = orange.ContinuousVariable("m2")
        d.addmeta(mid2, nf)
        e[nf] = 6.28
        self.assertEqual(e[nf], 6.28)
        self.assertEqual(e[mid2], 6.28)
        self.assertEqual(e["m2"], 6.28)

        mid3 = orange.newmetaid()
        sf = orange.StringVariable("m3")
        d.addmeta(mid3, sf)
        e["m3"] = "pixies"
        self.assertEqual(e[sf], "pixies")
        self.assertEqual(e[mid3], "pixies")
        self.assertEqual(e["m3"], "pixies")

        self.assertEqual(set(e.get_metas().values()), set([2.79, 6.28, "pixies"]))
        
        del e[mid1]
        with self.assertRaises(KeyError):
            e[mid1]
        self.assertEqual(set(e.get_metas().values()), set([6.28, "pixies"]))
        
        del e[mid3]
        with self.assertRaises(KeyError):
            e[sf]
        self.assertEqual(set(e.get_metas().values()), set([6.28]))

        del e[mid2]
        with self.assertRaises(KeyError):
            e["m2"]
        self.assertEqual(set(e.get_metas().values()), set([]))

        with self.assertRaises(TypeError):
            e[mid1] = "3.15"
        with self.assertRaises(TypeError):
            e[mid3] = 3.15

        with self.assertRaises(TypeError):
            e.set_meta(mid1, "3.15")
        with self.assertRaises(TypeError):
            e.set_meta(mid3 = 3.15)

        e.set_meta(mid1, 3.15)
        e.set_meta("m2", 3.16)
        e.set_meta(sf, "3.17")

        self.assertTrue(e.has_meta(mid1))
        self.assertTrue(e.has_meta(nf))
        self.assertTrue(e.has_meta(sf))

        e.remove_meta(sf)
        self.assertTrue(e.has_meta(mid1))
        self.assertTrue(e.has_meta(nf))
        self.assertFalse(e.has_meta(sf))

        e.remove_meta(mid1)
        self.assertFalse(e.has_meta(mid1))
        self.assertTrue(e.has_meta(nf))
        self.assertFalse(e.has_meta(sf))

        e.remove_meta("m2")        
        self.assertFalse(e.has_meta(mid1))
        self.assertFalse(e.has_meta(nf))
        self.assertFalse(e.has_meta(sf))
        
    def test_pickle(self):
        import pickle
        d = orange.ExampleTable("iris")
        e = d[0]
        self.assertRaises(pickle.PicklingError, pickle.dumps, e)
        e = orange.Example(e)
        s = pickle.dumps(e)
        e2 = pickle.loads(s)
        self.assertEqual(e, e)
        self.assertEqual(e, e2)
        
        id = orange.newmetaid()
        e[id] = 33
        d.domain.addmeta(id, orange.ContinuousVariable("x"))
        id2 = orange.newmetaid()
        d.domain.addmeta(id2, orange.StringVariable("y"))
        e[id2] = "foo"
        s = pickle.dumps(e)
        e2 = pickle.loads(s)
        self.assertEqual(e, e2)
        self.assertEqual(e2[id], 33)
        self.assertEqual(e2[id2], "foo")
        
        
        
if __name__ == "__main__":
    unittest.main()