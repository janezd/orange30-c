import unittest
from Orange.data import Table, Example, Unknown

class TableTestCase(unittest.TestCase):
    def setUp(self):
        pass


    def test_loading(self):
        d = Table("iris")
        self.assertEqual(len(d), 150)
        self.tnames(d.domain, vars)

        
    def tnames(self, domain, varnames):
        self.assertEqual([x.name for x in domain.variables], varnames)
        self.assertEqual([x.name for x in domain.attributes], varnames[:-1])
        self.assertEqual(domain.class_var.name, varnames[-1])
        
    def test_loading(self):
        d = Table("iris")
        f = open("iris.tab")
        vars = f.readline().strip().split("\t")
        f.close()
        self.assertEqual(len(d), 150)
        self.tnames(d.domain, vars)

    def test_findfile(self):
        d = Table("test1")
        self.assertEqual(d.name, "test1")
        d = Table("test1.tab")
        self.assertEqual(d.name, "test1")
        self.assertRaises(IOError, Table, "test1.txt")
        self.assertRaises(IOError, Table, "test_wrong.tab")
        self.assertRaises(IOError, Table, "test_wrong")
            

    def test_loading2(self):
        d = Table("test1")
        self.assertEqual([x.name for x in d.domain.attributes], list("abce"))
        self.assertEqual(d.domain.class_var.name, "d")
        
        self.assertEqual([e[0] for e in d], ["A", "B", "C"])
        
        # This line also tests whether values do an approximate comparison (function values_compare)
        self.assertEqual([e[1] for e in d], [0, 1.1, 2.22])
        
        self.assertEqual(d.domain[2].values, ["1", "0"])
        self.assertEqual([e[2] for e in d], ["0", "1", "1"])
        self.assertEqual([e[2] for e in d], [1, 0, 0])
        
        self.assertEqual(d.domain.class_var.values, ["f", "t"])
        self.assertEqual([e.get_class() for e in d], ["t", "t", "f"])

    def test_indexing_class(self):        
        d = Table("test1")
        self.assertEqual([e.get_class() for e in d], ["t", "t", "f"])
        cind = len(d.domain)-1
        self.assertEqual([e[cind] for e in d], ["t", "t", "f"])
        self.assertEqual([e["d"] for e in d], ["t", "t", "f"])
        cvar = d.domain.class_var
        self.assertEqual([e[cvar] for e in d], ["t", "t", "f"])

    def test_indexing(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")

            # regular, discrete
            varc = d.domain["c"]
            self.assertEqual(d[0, 1], "0")
            self.assertEqual(d[0, varc], "0")
            self.assertEqual(d[0, "c"], "0")
            self.assertEqual(d[0][1], "0")
            self.assertEqual(d[0][varc], "0")
            self.assertEqual(d[0]["c"], "0")

            # regular, continuous
            varb = d.domain["b"]
            self.assertEqual(d[0, 0], 0)
            self.assertEqual(d[0, varb], 0)
            self.assertEqual(d[0, "b"], 0)
            self.assertEqual(d[0][0], 0)
            self.assertEqual(d[0][varb], 0)
            self.assertEqual(d[0]["b"], 0)
            
            #negative
            varb = d.domain["b"]
            self.assertEqual(d[-2, 0], 3.333)
            self.assertEqual(d[-2, varb], 3.333)
            self.assertEqual(d[-2, "b"], 3.333)
            self.assertEqual(d[-2][0], 3.333)
            self.assertEqual(d[-2][varb], 3.333)
            self.assertEqual(d[-2]["b"], 3.333)

            # meta, discrete
            vara = d.domain["a"]
            metaa = d.domain.index("a")
            self.assertEqual(d[0, metaa], "A")
            self.assertEqual(d[0, vara], "A")
            self.assertEqual(d[0, "a"], "A")
            self.assertEqual(d[0][metaa], "A")
            self.assertEqual(d[0][vara], "A")
            self.assertEqual(d[0]["a"], "A")

            # meta, string
            vare = d.domain["e"]
            metae = d.domain.index("e")
            self.assertEqual(d[0, metae], "i")
            self.assertEqual(d[0, vare], "i")
            self.assertEqual(d[0, "e"], "i")
            self.assertEqual(d[0][metae], "i")
            self.assertEqual(d[0][vare], "i")
            self.assertEqual(d[0]["e"], "i")


    def test_indexing_example(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")
            e = d[0]

            # regular, discrete
            varc = d.domain["c"]
            self.assertEqual(e[1], "0")
            self.assertEqual(e[varc], "0")
            self.assertEqual(e["c"], "0")
            
            # regular, continuous
            varb = d.domain["b"]
            self.assertEqual(e[0], 0)
            self.assertEqual(e[varb], 0)
            self.assertEqual(e["b"], 0)

            # meta, discrete
            vara = d.domain["a"]
            metaa = d.domain.index("a")
            self.assertEqual(e[metaa], "A")
            self.assertEqual(e[vara], "A")
            self.assertEqual(e["a"], "A")

            # meta, string
            vare = d.domain["e"]
            metae = d.domain.index("e")
            self.assertEqual(e[metae], "i")
            self.assertEqual(e[vare], "i")
            self.assertEqual(e["e"], "i")
            

    def test_indexing_assign_value(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")

            # meta
            vara = d.domain["a"]
            metaa = d.domain.index("a")

            self.assertEqual(d[0, "a"], "A")
            d[0, "a"] = "B"
            self.assertEqual(d[0, "a"], "B")
            d[0]["a"] = "A"
            self.assertEqual(d[0, "a"], "A")
            
            d[0, vara] = "B"
            self.assertEqual(d[0, "a"], "B")
            d[0][vara] = "A"
            self.assertEqual(d[0, "a"], "A")
            
            d[0, metaa] = "B"
            self.assertEqual(d[0, "a"], "B")
            d[0][metaa] = "A"
            self.assertEqual(d[0, "a"], "A")
            
            # regular
            varb = d.domain["b"]

            self.assertEqual(d[0, "b"], 0)
            d[0, "b"] = 42
            self.assertEqual(d[0, "b"], 42)
            d[0]["b"] = 0
            self.assertEqual(d[0, "b"], 0)
            
            d[0, varb] = 42
            self.assertEqual(d[0, "b"], 42)
            d[0][varb] = 0
            self.assertEqual(d[0, "b"], 0)
            
            d[0, 0] = 42
            self.assertEqual(d[0, "b"], 42)
            d[0][0] = 0
            self.assertEqual(d[0, "b"], 0)


    def test_indexing_del_example(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")
            initlen = len(d)

            # remove first
            d[4, "e"] = "4ex"
            self.assertEqual(d[4, "e"], "4ex")
            del d[0]
            self.assertEqual(len(d), initlen-1)
            self.assertEqual(d[3, "e"], "4ex")

            # remove middle
            del d[2]
            self.assertEqual(len(d), initlen-2)
            self.assertEqual(d[2, "e"], "4ex")

            # remove middle
            del d[4]
            self.assertEqual(len(d), initlen-3)
            self.assertEqual(d[2, "e"], "4ex")

            # remove last
            d[-1, "e"] = "was last"
            del d[-1]
            self.assertEqual(len(d), initlen-4)
            self.assertEqual(d[2, "e"], "4ex")
            self.assertNotEqual(d[-1, "e"], "was last")

            # remove one before last
            d[-1, "e"] = "was last"
            del d[-2]
            self.assertEqual(len(d), initlen-5)
            self.assertEqual(d[2, "e"], "4ex")
            self.assertEqual(d[-1, "e"], "was last")

            with self.assertRaises(ValueError):
                del d[100]
            self.assertEqual(len(d), initlen-5)

            with self.assertRaises(ValueError):
                del d[-100]
            self.assertEqual(len(d), initlen-5)


    def test_indexing_assign_example(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")

            vara = d.domain["a"]
            metaa = vara.default_meta_id

            self.assertTrue(d[0].has_meta("a"))
            d[0] = ["3.14", "1", "f"]
            self.assertEqual(list(d[0]), [3.14, "1", "f"])
            self.assertFalse(d[0].has_meta("a"))
            d[0] = [3.15, 1, "t"]
            self.assertEqual(list(d[0]), [3.15, "0", "t"])

            with self.assertRaises(ValueError):
                d[0] = ["3.14", "1"]

            ex = orange.Example(d.domain, ["3.16", "1", "f"])
            d[0] = ex
            self.assertEqual(list(d[0]), [3.16, "1", "f"])

            ex = orange.Example(d.domain, ["3.16", "1", "f"])
            ex.set_meta("e", "mmmapp")
            d[0] = ex
            self.assertEqual(list(d[0]), [3.16, "1", "f"])
            ex.set_meta("e", "mmmapp")

    def test_slice(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")
            x = d[:3]
            self.assertEqual(len(x), 3)
            self.assertEqual([e[0] for e in x], [0, 1.1, 2.22])

            x = d[2:5]
            self.assertEqual(len(x), 3)
            self.assertEqual([e[0] for e in x], [2.22, 2.23, 2.24])

            x = d[4:1:-1]
            self.assertEqual(len(x), 3)
            self.assertEqual([e[0] for e in x], [2.24, 2.23, 2.22])

            x = d[-3:]
            self.assertEqual(len(x), 3)
            self.assertEqual([e[0] for e in x], [2.26, 3.333, Unknown])


    def test_assign_slice_value(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")
            d[2:5, 0] = 42
            self.assertEqual([e[0] for e in d], [0, 1.1, 42, 42, 42, 2.25, 2.26, 3.333, Unknown])
            d[:3, "b"] = 43
            self.assertEqual([e[0] for e in d], [43, 43, 43, 42, 42, 2.25, 2.26, 3.333, None])
            d[-2:, d.domain[0]] = 44
            self.assertEqual([e[0] for e in d], [43, 43, 43, 42, 42, 2.25, 2.26, 44, 44])

            d[2:5, "a"] = "A"            
            self.assertEqual([e["a"] for e in d], list("ABAAACCDE"))


    def test_del_slice_example(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")

            vals = [e[0] for e in d]

            del d[2:2]
            self.assertEqual([e[0] for e in d], vals)
            
            del d[2:5]
            del vals[2:5]
            self.assertEqual([e[0] for e in d], vals)

            del d[:]
            self.assertEqual(len(d), 0)
            
        
    def test_set_slice_example(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")
            d[5, 0]  = 42
            d[:3] = d[5]
            self.assertEqual(d[1, 0], 42)

            d[5:2:-1] = [3, None, None]
            self.assertEqual([e[0] for e in d], [42, 42, 42, 3, 3, 3, 2.26, 3.333, None])
            self.assertTrue(d[3, 2].is_undefined())
            
            with self.assertRaises(TypeError):
                d[2:5] = 42

    def test_multiple_indices(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")

            with self.assertRaises(IndexError):
                d[2, 5, 1]
                
            with self.assertRaises(IndexError):
                x = d[(2, 5, 1)]

            x = d[[2, 5, 1]]
            self.assertEqual([e[0] for e in x], [2.22, 2.25, 1.1])
# TODO reenable tests with metas when they are implemented
#            self.assertEqual([e["a"] for e in x], ["C", "C", "B"])

            x = d[(x for x in range(5, 2, -1))]
            self.assertEqual([e[0] for e in x], [2.25, 2.24, 2.23])


    def test_assign_multiple_indices_value(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")

            d[1:4, "b"] = 42
            self.assertEqual([e[0] for e in d], [0, 42, 42, 42, 2.24, 2.25, 2.26, 3.333, None])

            d[range(5, 2, -1), "b"] = None
            print(d[4, d.domain[0]], type(d[4, d.domain[0]]))
            self.assertEqual([e[d.domain[0]] for e in d], [0, 42, 42, None, "?", "", 2.26, 3.333, None])


    def test_del_multiple_indices_example(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")

            vals = [e[0] for e in d]

            del d[[1, 5, 2]]
            del vals[5]
            del vals[2]
            del vals[1]
            self.assertEqual([e[0] for e in d], vals)

            del d[range(1, 3)]
            del vals[1:3]
            self.assertEqual([e[0] for e in d], vals)
            
            
    def test_set_multiple_indices_example(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            d = Table("test2")

            vals = [e[0] for e in d]
            d[[1, 2, 5]] = [42, None, None]
            vals[1]=vals[2]=vals[5]=42
            self.assertEqual([e[0] for e in d], vals)


    def test_views(self):
        d = Table("zoo")
        crc = d.checksum(True)
        x = d[:20]
        self.assertEqual(crc, d.checksum(True))
        del x[13]
        self.assertEqual(crc, d.checksum(True))
        del x[4:9]
        self.assertEqual(crc, d.checksum(True))
        x[2, "name"] = "leocorn"
        self.assertEqual(d[2, "name"], "leocorn")
        x[2, 1] = 0
        self.assertEqual(d[2, 1], 0)
        x[2, 1] = 1
        self.assertEqual(d[2, 1], 1)
        x[2][1] = 0
        self.assertEqual(d[2, 1], 0)
        
        d[2][1] = 1
        self.assertEqual(x[2, 1], 1)
        d[2, 1] = 0
        self.assertEqual(x[2, 1], 0)
        
        x[2, "name"] = "dinosaur"
        self.assertEqual(d[2, "name"], "dinosaur")
        x[2]["name"] = "yeti"
        self.assertEqual(d[2, "name"], "yeti")

        d[2, "name"] = "dinosaur"
        self.assertEqual(x[2, "name"], "dinosaur")
        d[2]["name"] = "yeti"
        self.assertEqual(x[2, "name"], "yeti")
        
        
    def test_bool(self):
        d = Table("iris")
        self.assertTrue(d)
        del d[:]
        self.assertFalse(d)

        d = Table("test3")
        self.assertFalse(d)

        d = Table("iris")
        self.assertTrue(d)
        d.clear()
        self.assertFalse(d)
        

    def test_checksum(self):
        d = Table("zoo")
        d[42,3] = 0
        crc1  = d.checksum()
        d[42,3] = 1
        crc2  = d.checksum()
        self.assertNotEqual(crc1, crc2)
        d[42,3] = 0
        crc3  = d.checksum()
        self.assertEqual(crc1, crc3)
        oldname = d[42, "name"]
        d[42, "name"] = "non-animal"
        crc4  = d.checksum()
        self.assertEqual(crc1, crc4)
        crc4  = d.checksum(True)
        crc5  = d.checksum(1)
        crc6  = d.checksum(False)
        self.assertNotEqual(crc1, crc4)
        self.assertNotEqual(crc1, crc5)
        self.assertEqual(crc1, crc6)

    def test_random(self):
        d = Table("zoo")
        self.assertTrue(isinstance(d.random_example(), Example))
        d.clear()
        self.assertRaises(IndexError, d.random_example)

    def test_total_weight(self):
        d = Table("zoo")
        self.assertEqual(d.total_weight(), len(d))

        d.set_weights(0)
        d[0].set_weight(0.1)
        d[10].set_weight(0.2)
        d[-1].set_weight(0.3)
        self.assertAlmostEqual(d.total_weight(), 0.6)
        del d[10]
        self.assertAlmostEqual(d.total_weight(), 0.4)
        d.clear()
        self.assertAlmostEqual(d.total_weight(), 0)
        

    def test_has_missing(self):
        d = Table("zoo")
        self.assertFalse(d.has_missing())
        self.assertFalse(d.has_missing_class())

        d[10, 3] = "?"
        self.assertTrue(d.has_missing())
        self.assertFalse(d.has_missing_class())
        
        d[10].set_class("?")
        self.assertTrue(d.has_missing())
        self.assertTrue(d.has_missing_class())

        d = Table("test3")
        self.assertFalse(d.has_missing())
        self.assertFalse(d.has_missing_class())
        
    def test_shuffle(self):
        d = Table("zoo")
        crc = d.checksum()
        names = set(str(x["name"]) for x in d)
        
        d.shuffle()
        self.assertNotEqual(crc, d.checksum())
        self.assertSetEqual(names, set(str(x["name"]) for x in d))
        crc2 = d.checksum()

        x = d[2:10]
        crcx = x.checksum()
        d.shuffle()
        self.assertNotEqual(crc2, d.checksum())
        self.assertEqual(crcx, x.checksum())

        crc2 = d.checksum()
        x.shuffle()
        self.assertNotEqual(crcx, x.checksum())
        self.assertEqual(crc2, d.checksum())

    @staticmethod
    def not_less_ex(ex1, ex2):
        for v1, v2 in zip(ex1, ex2):
            if v1 != v2:
                return v1 < v2
        return True

    @staticmethod
    def sorted(d):
        for i in range(1, len(d)):
            if not ExampleTableTestCase.not_less_ex(d[i-1], d[i]):
                return False
        return True
    
    @staticmethod
    def not_less_ex_ord(ex1, ex2, ord):
        for a in ord:
            if ex1[a] != ex2[a]:
                return ex1[a] < ex2[a]
        return True

    @staticmethod
    def sorted_ord(d, ord):
        for i in range(1, len(d)):
            if not ExampleTableTestCase.not_less_ex_ord(d[i-1], d[i], ord):
                return False
        return True
    
    def test_sort(self):
        d = Table("iris")
        d.sort()
        self.assertTrue(ExampleTableTestCase.sorted(d))

        for order in ([1], [], [4, 1, 2]):
            d.sort(order)
            self.assertTrue(ExampleTableTestCase.sorted_ord(d, order))

        e = d[0]
        self.assertRaises(RuntimeError, d.shuffle)
        self.assertRaises(RuntimeError, d.sort)
        del e
        
        d.shuffle()
        order = [4, 1, 2]
        crc = d.checksum()
        x = d[:]
        x.sort()
        self.assertEqual(crc, d.checksum())
        self.assertTrue(ExampleTableTestCase.sorted(x))

        x.sort(order)
        self.assertEqual(crc, d.checksum())
        self.assertTrue(ExampleTableTestCase.sorted_ord(x, order))

    def test_remove_duplicates(self):
        d = Table("zoo")
        d.remove_duplicates()
        d.sort()
        for i in range(1, len(d)):
            self.assertNotEqual(d[i-1].native(), d[i].native())

    def test_append(self):
        d = Table("test3")
        d.append([None]*3)
        self.assertEqual(1, len(d))
        self.assertEqual(d[0].native(), [None, None, None])

        d.append([42, "0", None])
        self.assertEqual(2, len(d))
        self.assertEqual(d[1].native(), [42, "0", None])

    def test_append2(self):
        d = Table("iris")
        d.shuffle()
        l1 = len(d)
        d.append([1, 2, 3, 4, 0])
        self.assertEqual(len(d), l1+1)
        self.assertEqual(d[-1], [1, 2, 3, 4, 0])

        x = orange.Example(d[10])
        d.append(x)        
        self.assertEqual(d[-1], d[10])

        x = d[:50]
        x.append(d[50])
        self.assertEqual(x[50], d[50])
                 
    def test_extend(self):
        d = Table("iris")
        d.shuffle()

        x = d[:5]
        self.assertRaises(RuntimeError, d.extend, x)

        x.extend(d[5:10])
        for de, xe in zip(d, x):
            self.assertTrue(de==xe)

        x = d[:5]
        x.extend(x)        
        self.assertTrue(all(x[i]==x[5+i] for i in range(5)))

    def test_convert_through_append(self):
        d = Table("iris")
        dom2 = orange.Domain([d.domain[0], d.domain[2], d.domain[4]])
        d2 = Table(dom2)
        dom3 = orange.Domain([d.domain[1], d.domain[2]], None)
        d3 = Table(dom3)
        for e in d[:5]:
            d2.append(e)
            d3.append(e)
        for e, e2, e3 in zip(d, d2, d3):
            self.assertEqual(e[0], e2[0])
            self.assertEqual(e[1], e3[0])

    def test_pickle(self):
        import pickle

        d = Table("zoo")
        s = pickle.dumps(d)
        d2 = pickle.loads(s)
        self.assertEqual(d[0], d2[0])
        self.assertEqual(d.checksum(), d2.checksum())

        d = Table("iris")
        s = pickle.dumps(d)
        d2 = pickle.loads(s)
        self.assertEqual(d[0], d2[0])
        self.assertEqual(d.checksum(), d2.checksum())

    def test_pickle_ref(self):
        import pickle

        d = Table("zoo")
        dr = d[:10]
        self.assertEqual(dr.base, d)

        s = pickle.dumps(dr)
        d2 = pickle.loads(s)
        self.assertEqual(d[0], d2[0])
        self.assertEqual(dr.checksum(), d2.checksum())


    def test_sample(self):
        d = Table("iris")
        
        li = [1]*10+[0]*140
        d1 = d.sample(li)
        self.assertEqual(len(d1), 10)
        for i in range(10):
            self.assertEqual(d1[i], d[i])
            self.assertEqual(d1[i].id, d[i].id)
        d[0, 0] = 42
        self.assertEqual(d1[0,0], 42)

        d1 = d.sample(li, copy=True)
        self.assertEqual(len(d1), 10)
        self.assertEqual(d1[0], d[0])
        self.assertNotEqual(d1[0].id, d[0].id)
        d[0, 0] = 41
        self.assertEqual(d1[0,0], 42)
        
        li = [1, 2, 3, 4, 5]*30
        d1 = d.sample(li, 2)
        self.assertEqual(len(d1), 30)
        for i in range(30):
            self.assertEqual(d1[i].id, d[1+5*i].id)
            
        ri = orange.MakeRandomIndicesCV(d)
        for fold in range(10):
            d1 = d.sample(ri, fold)
            self.assertEqual(orange.get_class_distribution(d1), [5, 5, 5])

    def test_translate(self):
        d = Table("iris")
        d_ref = d.translate([1, 2])
        self.assertEqual(d[0].id, d_ref[0].id)
        d[0, 1] = 42
        self.assertEqual(d[0, 1], 42)
        self.assertEqual(d_ref[0, 0], 42)
        d_refcopy = Table(d_ref)
        d_refslice = d_ref[:5]
        self.assertNotEqual(d[0].id, d_refcopy[0].id)
        self.assertEqual(d[0].id, d_refslice[0].id)
        self.assertEqual(d_refcopy[0, 0], 42)
        self.assertEqual(d_refslice[0, 0], 42)
        d[0, 1] = 43
        self.assertEqual(d[0, 1], 43)
        self.assertEqual(d_ref[0, 0], 43)
        self.assertEqual(d_refcopy[0, 0], 42)
        self.assertEqual(d_refslice[0, 0], 43)
        d_refslice[0, 0] = 44
        self.assertEqual(d[0, 1], 44)
        self.assertEqual(d_ref[0, 0], 44)
        self.assertEqual(d_refcopy[0, 0], 42)
        self.assertEqual(d_refslice[0, 0], 44)
        e = d_ref[0]
        e[0] = 45
        self.assertEqual(e.reference_type, orange.Example.ReferenceType.Indirect)
        self.assertEqual(d[0, 1], 45)
        self.assertEqual(d_ref[0, 0], 45)
        self.assertEqual(d_refcopy[0, 0], 42)
        self.assertEqual(d_refslice[0, 0], 45)

        
    def test_translate_through_slice(self):
        d = Table("iris")
        d_ref = d[:,(1, 2)]
        self.assertEqual(d[0].id, d_ref[0].id)
        d[0, 1] = 42
        self.assertEqual(d[0, 1], 42)
        self.assertEqual(d_ref[0, 0], 42)

        d_ref = d[5:2:-1,("petal length", "sepal length")]
        self.assertEqual(len(d_ref), 3)
        self.assertEqual(d[5].id, d_ref[0].id)
        d_ref[0, "petal length"] = 42
        self.assertEqual(d[5, "petal length"], 42)
        d[5, "petal length"] = 43
        self.assertEqual(d_ref[0, "petal length"], 43)

        dom = orange.Domain(["petal length", "sepal length", "iris"], source=d.domain)
        d_ref = d[:10, dom]
        self.assertEqual(d_ref.domain.class_var, d.domain.class_var)
        d_ref[0, "petal length"] = 44
        self.assertEqual(d[0, "petal length"], 44)
        d[0, "petal length"] = 45
        self.assertEqual(d_ref[0, "petal length"], 45)


    def test_indexing_filter_cont(self):
        d = Table("iris")
        v = d.columns
        d2 = d[v.sepal_length < 5]
        self.assertEqual(len(d2), 22)
        d2[0, 0] = 42
        d3 = [e for e in d if e[0] == 42]
        self.assertEqual(len(d3), 1)
        self.assertEqual(d3[0], d2[0])

        d = Table("iris")
        v = d.columns
        self.assertEqual(len(d[v.sepal_length <= 5]), 32)
        self.assertEqual(len(d[v.sepal_length > 5]), 118)
        self.assertEqual(len(d[v.sepal_length >= 5]),128)
        self.assertEqual(len(d[v.sepal_length == 5]), 10)

        self.assertEqual(len(d[(v.sepal_length == 5) & (v.sepal_width <3.3)]), 4)
        self.assertEqual(len(d[(v.sepal_length < 4.5) | (v.sepal_width == 3.0)]), 28)

        self.assertEqual(len(d[v.sepal_width == (3.0, 3.3)]), 57)
        self.assertEqual(len(d[v.sepal_width != (3.0, 3.3)]), 93)
        self.assertEqual(len(d[v.sepal_length < (5.0, 5.3)]), 22)
        self.assertEqual(len(d[v.sepal_length <= (5.0, 5.3)]), 32)
        self.assertEqual(len(d[v.sepal_length > (4.7, 5.0)]), 118)
        self.assertEqual(len(d[v.sepal_length >= (4.7, 5.0)]), 128)

    def test_indexing_filter_disc(self):
        d = Table("iris")
        v = d.columns
        self.assertEqual(len(d[v.iris == "Iris-setosa"]), 50)
        self.assertEqual(len(d[v.iris == ["Iris-setosa", "Iris-versicolor"]]), 100)
        self.assertEqual(len(d[v.iris != "Iris-setosa"]), 100)
        self.assertEqual(len(d[v.iris != ["Iris-setosa", "Iris-versicolor"]]), 50)
        with self.assertRaises(TypeError):
            d[v.iris < "Iris-setosa"]

    def test_indexing_filter_string(self):
        d = Table("zoo")
        name = d.domain["name"]
        self.assertEqual(len(d[name == "girl"]), 1)
        self.assertEqual(len(d[name != "girl"]), 100)
        self.assertEqual(len(d[name == ["girl", "gull", "mole"]]), 3)
        self.assertEqual(len(d[name != ["girl", "gull", "mole"]]), 98)

        self.assertEqual(len(d[name < "calf"]), 6)
        self.assertEqual(len(d[name <= "calf"]), 7)
        self.assertEqual(len(d[name > "calf"]), 94)
        self.assertEqual(len(d[name >= "calf"]), 95)

        with self.assertRaises(TypeError):
            d[name < ["calf", "girl"]]

    def test_indexing_filter_assign(self):
        d = Table("iris")
        v = d.columns
        d[v.iris == "Iris-setosa", v.petal_length] = 42
        for e in d:
            if e[-1] == "Iris-setosa":
                self.assertEqual(e["petal length"], 42)
            else:
                self.assertNotEqual(e["petal length"], 42)

        """
        # This does not work yet for ass_subscript in general, not only for filters

        d = Table("iris")
        v = d.columns
        d[v.iris == "Iris-setosa", (v.petal_length, v.sepal_length)] = 42
        for e in d:
            if e[-1] == "Iris-setosa":
                self.assertEqual(e["petal length"], 42)
                self.assertEqual(e["sepal length"], 42)
            else:
                self.assertNotEqual(e["petal length"], 42)
                self.assertNotEqual(e["sepal length"], 42)
            self.assertNotEqual(e["petal width"], 42)
            self.assertNotEqual(e["sepal width"], 42)
         """

        d = Table("iris")
        v = d.columns
        del d[v.sepal_length <= 5]
        self.assertEqual(len(d), 118)
        for e in d:
            self.assertGreater(e["sepal length"], 5)


    def test_saveTab(self):
        d = Table("iris")[:3]
        d.save("test-save.tab")
        try:
            d2 = Table("test-save.tab")
            for e1, e2 in zip(d, d2):
                self.assertEqual(e1, e2)
        finally:
            os.remove("test-save.tab")

        dom = orange.Domain([orange.ContinuousVariable("a")])
        d = Table(dom)
        d += [[i] for i in range(3)]
        d.save("test-save.tab")
        try:
            d2 = Table("test-save.tab")
            self.assertEqual(len(d.domain.attributes), 0)
            self.assertEqual(d.domain.classVar, dom[0])
            for i in range(3):
                self.assertEqual(d2[i], [i])
        finally:
            os.remove("test-save.tab")

        dom = orange.Domain([orange.ContinuousVariable("a")], None)
        d = Table(dom)
        d += [[i] for i in range(3)]
        d.save("test-save.tab")
        try:
            d2 = Table("test-save.tab")
            self.assertEqual(len(d.domain.attributes), 1)
            self.assertEqual(d.domain[0], dom[0])
            for i in range(3):
                self.assertEqual(d2[i], [i])
        finally:
            os.remove("test-save.tab")


    def test_from_numpy(self):
        import random
        a = numpy.arange(20, dtype="d").reshape((4, 5))
        a[:,-1] = [0, 0, 0, 1]
        dom = orange.Domain([orange.ContinuousVariable(x) for x in "abcd"],
                             orange.DiscreteVariable("e", values=["no", "yes"]))
        table = Table(dom, a)
        for i in range(4):
            self.assertEqual(table[i].getclass(), "no" if i < 3 else "yes")
            for j in range(5):
                self.assertEqual(a[i, j], table[i, j])
                table[i, j] = random.random()
                self.assertEqual(a[i, j], table[i, j])

        with self.assertRaises(ValueError):
            table[0, orange.newmetaid()] = 5

    def test_anonymous_from_numpy(self):
        a = numpy.zeros((4, 5))
        a[0, 0] = 0.5
        a[3, 1] = 11
        a[3, 2] = -1
        a[1, 3] = 5
        knn = orange.kNNLearner(a)
        data = knn.find_nearest.examples
        domain = data.domain
        self.assertEqual([var.name for var in domain.variables],
            ["var%05i" % i for i in range(5)])
        self.assertEqual(domain[0].var_type, orange.VarTypes.Continuous)
        self.assertEqual(domain[1].var_type, orange.VarTypes.Continuous)
        self.assertEqual(domain[2].var_type, orange.VarTypes.Continuous)
        self.assertEqual(domain[3].var_type, orange.VarTypes.Discrete)
        self.assertEqual(domain[3].values, ["v%i" % i for i in range(6)])
        self.assertEqual(domain[4].var_type, orange.VarTypes.Discrete)
        self.assertEqual(domain[4].values, ["v0"])

        dom2 = orange.Domain([orange.ContinuousVariable("a"),
                              orange.ContinuousVariable("b"),
                              orange.ContinuousVariable("c"),
                              orange.DiscreteVariable("d", values="abcdef"),
                              orange.DiscreteVariable("e", values="ab")])
        ex = orange.Example(dom2, [3.14, 42, 2.7, "d", "a"])
        knn(ex)
        ex1 = domain(ex)
        self.assertEqual(list(map(float, ex1)), list(map(float, ex)))
        ex2 = dom2(ex1)
        self.assertEqual(list(map(float, ex1)), list(map(float, ex2)))
        self.assertEqual(ex1, ex2)
        self.assertEqual(ex1, ex)

        iris = Table("iris")
        with self.assertRaises(ValueError):
            domain(iris[0])

        knn.find_nearest.examples = numpy.zeros((3, 3))
        domain = knn.find_nearest.examples.domain
        for i in range(3):
            var = domain[i]
            self.assertEqual(var.name, "var%05i" % i)
            self.assertEqual(var.values, ["v0"])


if __name__ == "__main__":
#    suite = unittest.TestLoader().loadTestsFromName("__main__.ExampleTableTestCase.test_random")
#    unittest.TextTestRunner(verbosity=2).run(suite)
    unittest.main()