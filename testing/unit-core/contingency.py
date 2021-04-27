import unittest
import orange
import pickle

class Contingency(unittest.TestCase):
    def setUp(self):
        pass

    
    def test_attrClass_disc(self):
        d = orange.ExampleTable("zoo")
        cd = orange.get_class_distribution(d)
        ad = orange.Distribution(0, d)
        cont = orange.ContingencyAttrClass(0, d)
        self.assertEqual(cont.inner_distribution, cd)
        self.assertEqual(cont.outer_distribution, ad)
        self.assertNotEqual(cont[0], cont[1])
        self.assertEqual(id(cont[0]), id(cont[d.domain[0].values[0]]))
        self.assertEqual(len(cont), len(ad))
        for cc in cont:
            self.assertEqual(cc, cont[0])
            break
        self.assertEqual(cont.keys(), d.domain[0].values)
        self.assertEqual(cont.values()[0], cont[0])
        self.assertEqual(cont.values()[1], cont[1])
        k, v = cont.items()[0]
        self.assertEqual(k, d.domain[0].values[0])
        self.assertEqual(v, cont[0])
     
        s = pickle.dumps(cont)
        cont2 = pickle.loads(s)

        self.assertEqual(cont.innerDistribution, cont2.innerDistribution)
        self.assertEqual(cont.innerVariable, cont2.innerVariable)
        self.assertEqual(cont.outerDistribution, cont2.outerDistribution)
        self.assertEqual(cont.outerVariable, cont2.outerVariable)
        self.assertEqual(cont[0], cont2[0])
        self.assertEqual(cont[1], cont2[1])
        
        cont.normalize()
        self.assertAlmostEqual(sum(cont.p_class(0)), 1.0)
        self.assertEqual(cont.p_class(0)[0], cont.p_class(0, 0))
        self.assertEqual(cont.p_class(0)[0], cont2.p_class(0)[0]/cont2.p_class(0).abs)

        x = cont[0][0]
        cont.add_var_class(0, 0, 0.5)
        self.assertEqual(x+0.5, cont[0][0])
        
        self.assertEqual(cont[0][0], cont[0,0])
        self.assertEqual(cont[d.domain[0].values[0], d.domain.classVar.values[0]], cont[0,0])
        
        with self.assertRaises(IndexError):
            cont["?"]
            

    def test_attrClass_cont(self):
        d = orange.ExampleTable("iris")
        cd = orange.get_class_distribution(d)
        ad = orange.Distribution(0, d)
        cont = orange.ContingencyAttrClass(0, d)
        fv = cont.keys()[0]
        self.assertEqual(cont.inner_distribution, cd)
        self.assertEqual(cont.outer_distribution, ad)
        self.assertEqual(len(cont), len(ad))
        s = set()
        for v in d:
            s.add(v[0])
        self.assertEqual(s, set(cont.keys()))
        self.assertEqual(len(d), sum(sum(v) for v in cont.values()))
        
        s = pickle.dumps(cont)
        cont2 = pickle.loads(s)
        self.assertEqual(cont.innerDistribution, cont2.innerDistribution)
        self.assertEqual(cont.innerVariable, cont2.innerVariable)
        self.assertEqual(cont.outerDistribution, cont2.outerDistribution)
        self.assertEqual(cont.outerVariable, cont2.outerVariable)
        self.assertEqual(cont[fv], cont2[fv])
        
        cont.normalize()
        self.assertAlmostEqual(sum(cont.p_class(fv)), 1.0)
        self.assertEqual(cont.p_class(fv)[0], cont.p_class(fv, 0))
        self.assertEqual(cont.p_class(fv)[0], cont2.p_class(fv)[0]/cont2.p_class(fv).abs)

        x = cont[0][0]
        cont.add_var_class(0, 0, 0.5)
        self.assertEqual(x+0.5, cont[0][0])
        
        self.assertEqual(cont[fv][0], cont[fv,0])
        
        with self.assertRaises(IndexError):
            cont["?"]
        
    def test_classAttr_disc(self):
        d = orange.ExampleTable("zoo")
        cd = orange.get_class_distribution(d)
        ad = orange.Distribution(0, d)
        cont = orange.ContingencyClassAttr(0, d)
        self.assertEqual(cont.inner_distribution, ad)
        self.assertEqual(cont.outer_distribution, cd)
        self.assertNotEqual(cont[0], cont[1])
        self.assertEqual(id(cont[0]), id(cont[d.domain.classVar.values[0]]))
        self.assertEqual(len(cont), len(cd))
        for cc in cont:
            self.assertEqual(cc, cont[0])
            break
        self.assertEqual(cont.keys(), d.domain.classVar.values)
        self.assertEqual(cont.values()[0], cont[0])
        self.assertEqual(cont.values()[1], cont[1])
        k, v = cont.items()[0]
        self.assertEqual(k, d.domain.classVar.values[0])
        self.assertEqual(v, cont[0])
        
        s = pickle.dumps(cont)
        cont2 = pickle.loads(s)
        self.assertEqual(cont.innerDistribution, cont2.innerDistribution)
        self.assertEqual(cont.innerVariable, cont2.innerVariable)
        self.assertEqual(cont.outerDistribution, cont2.outerDistribution)
        self.assertEqual(cont.outerVariable, cont2.outerVariable)
        self.assertEqual(cont[0], cont2[0])
        self.assertEqual(cont[1], cont2[1])
        
        cont.normalize()
        self.assertAlmostEqual(sum(cont.p_attr(0)), 1.0)
        self.assertEqual(cont.p_attr(0)[0], cont.p_attr(0, 0))
        self.assertEqual(cont.p_attr(0)[0], cont2.p_attr(0)[0]/cont2.p_attr(0).abs)

        x = cont[0][0]
        cont.add_var_class(0, 0, 0.5)
        self.assertEqual(x+0.5, cont[0][0])
        
        self.assertEqual(cont[0][0], cont[0,0])
        self.assertEqual(cont[d.domain.classVar.values[0], d.domain[0].values[0]], cont[0,0])
        
        with self.assertRaises(IndexError):
            cont["?"]
        
    def test_classAttr_cont(self):
        d = orange.ExampleTable("iris")
        cd = orange.get_class_distribution(d)
        ad = orange.Distribution(0, d)
        cont = orange.ContingencyClassAttr(0, d)
        fv = cont[0].keys()[0]
        self.assertEqual(cont.inner_distribution, ad)
        self.assertEqual(cont.outer_distribution, cd)
        self.assertEqual(len(cont), len(cd))
        
        s = pickle.dumps(cont)
        cont2 = pickle.loads(s)
        self.assertEqual(cont.innerDistribution, cont2.innerDistribution)
        self.assertEqual(cont.innerVariable, cont2.innerVariable)
        self.assertEqual(cont.outerDistribution, cont2.outerDistribution)
        self.assertEqual(cont.outerVariable, cont2.outerVariable)
        self.assertEqual(cont[0], cont2[0])
        
        cont.normalize()
        self.assertAlmostEqual(sum(cont.p_attr(0).values()), 1.0)
        self.assertEqual(cont.p_attr(0)[fv], cont.p_attr(fv, 0))
        self.assertEqual(cont.p_attr(0)[fv], cont2.p_attr(fv, 0)/cont2.p_attr(0).abs)

        x = cont[0][0]
        cont.add_var_class(0, 0, 0.5)
        self.assertEqual(x+0.5, cont[0][0])
        
        self.assertEqual(cont[fv][0], cont[fv,0])
        
        with self.assertRaises(IndexError):
            cont["?"]
        
    def test_domainContingency(self):
            d = orange.ExampleTable("zoo")
            dc = orange.DomainContingency(d)
            cd = orange.get_class_distribution(d)
            for i, c in enumerate(dc):
                self.assertEqual(id(d.domain.attributes[i]), id(c.outer_variable))
                self.assertEqual(id(dc[d.domain.attributes[i]]), id(c))
            self.assertFalse(dc.class_is_outer)
            self.assertEqual(dc.classes, cd)
            dc.normalize()
            self.assertEqual(sum(dc[0][0]), 1)

            dc.foo = 42
            s = pickle.dumps(dc)
            dc2 = pickle.loads(s)
            self.assertEqual(dc2.foo, dc.foo)
            self.assertEqual(dc2.classes, dc.classes)
            self.assertEqual(dc2[0].outer_variable, dc[0].outer_variable)

if __name__ == "__main__":
    unittest.main()