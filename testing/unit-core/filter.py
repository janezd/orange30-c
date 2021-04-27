import unittest
import orange
import pickle

class FilterTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_random(self):
        d = orange.ExampleTable("zoo")
        f = orange.Filter_random(prob=0.5)
        s = 0
        for i in range(10):
            for e in d:
                if f(e):
                    s += 1
        self.assertTrue(300 < s < 700) # unlikely to fail ;)
        f.negate = True
        s = 0
        for i in range(10):
            for e in d:
                if f(e):
                    s += 1
        self.assertTrue(300 < s < 700) # unlikely to fail ;)
        f.prob = 0.331
        s = pickle.dumps(f)
        f2 = pickle.loads(s)
        self.assertEqual(f2.prob, f.prob)


    def test_hasSpecial(self):
        d = orange.ExampleTable("zoo")
        f = orange.Filter_hasSpecial()

        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 0)

        for ii, i in enumerate([10, 20, 50]):
            d[i, ii] = "?"
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 3)
        for ii, i in enumerate([10, 20, 50]):
            self.assertEqual(d[i].id, d2[ii].id)

        f.negate = True
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), len(d)-3)

    def test_isDefined(self):
        d = orange.ExampleTable("zoo")
        f = orange.Filter_isDefined(negate=True)

        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 0)

        for ii, i in enumerate([10, 20, 50]):
            d[i, ii] = "?"
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 3)
        for ii, i in enumerate([10, 20, 50]):
            self.assertEqual(d[i].id, d2[ii].id)

        f.negate = False
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), len(d)-3)

        f.negate = True
        f.domain = d.domain
        f.check[d.domain[0]] = False
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 2)
        for ii, i in enumerate([20, 50]):
            self.assertEqual(d[i].id, d2[ii].id)

    def test_hasMeta(self):
        d = orange.ExampleTable("zoo")
        f = orange.Filter_hasMeta()
        f.id = d.domain.metaid("name")
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d), len(d2))

        f.id = orange.newmetaid()
        d[0, f.id] = 42
        d[1, f.id] = 42
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 2)

    def test_hasMeta(self):
        d = orange.ExampleTable("zoo")
        f = orange.Filter_hasMeta()
        f.id = d.domain.metaid("name")
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d), len(d2))

        f.id = orange.newmetaid()
        d[0, f.id] = 42
        d[1, f.id] = 42
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 2)

    def test_hasClassValue(self):
        d = orange.ExampleTable("zoo")
        f = orange.Filter_hasClassValue()
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d), len(d2))
        d[:5, -1] = "?"
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d)-5, len(d2))
        self.assertEqual(d[5].id, d2[0].id)

    def test_sameValue(self):
        d = orange.ExampleTable("zoo")
        f = orange.Filter_sameValue(position=2, value=1)
        for e in d:
            self.assertEqual(e[2]==1, f(e))

        d = orange.ExampleTable("iris")
        f.value = 5.1
        for e in d:
            self.assertEqual(e[2]==5.1, f(e))

        f.position = -1
        f.value = 0
        for e in d:
            self.assertEqual(e.getclass()==0, f(e))

    def test_valueFilter_continuous(self):
        d = orange.ExampleTable("iris")
        v = d.columns
        f = orange.ValueFilter_continuous(v.petal_length,
            orange.ValueFilter.Operator.Between, min=4.5, max=5.1)
        for e in d:
            self.assertEqual(4.5 <= e[2] <= 5.1, f(e))

        f.ref = 5.1
        f.oper=orange.ValueFilter.Operator.Equal
        for e in d:
            self.assertEqual(e[2]==5.1, f(e))

        f.oper = orange.ValueFilter.Operator.NotEqual
        for e in d:
            self.assertEqual(e[2]!=5.1, f(e))

        f.oper = orange.ValueFilter.Operator.Less
        for e in d:
            self.assertEqual(e[2]<5.1, f(e))

        f.oper = orange.ValueFilter.Operator.LessEqual
        for e in d:
            self.assertEqual(e[2]<=5.1, f(e))

        f.oper = orange.ValueFilter.Operator.Greater
        for e in d:
            self.assertEqual(e[2]>5.1, f(e))

        f.oper = orange.ValueFilter.Operator.GreaterEqual
        for e in d:
            self.assertEqual(e[2]>=5.1, f(e))

        f.oper = orange.ValueFilter.Operator.Between
        f.min, f.max = 4.5, 5.1
        for e in d:
            self.assertEqual(4.5 <= e[2] <= 5.1, f(e))

        f.oper = orange.ValueFilter.Operator.Outside
        f.min, f.max = 4.5, 5.1
        for e in d:
            self.assertEqual(4.5 <= e[2] <= 5.1, not f(e))


    def test_valueFilter_continuous_missargs(self):
        d = orange.ExampleTable("iris")
        v = d.columns
        f = orange.ValueFilter_continuous(v.petal_length, min=4.5, max=5.1)
        for e in d:
            self.assertEqual(4.5 <= e[2] <= 5.1, f(e))

        f = orange.ValueFilter_continuous(v.petal_length, min=4.5)
        for e in d:
            self.assertEqual(4.5 <= e[2], f(e))

        f = orange.ValueFilter_continuous(v.petal_length, max=5.1)
        for e in d:
            self.assertEqual(e[2] <= 5.1, f(e))

        self.assertRaises(TypeError, orange.ValueFilter_continuous,
                          v.petal_length, 4.5)

        self.assertRaises(ValueError, orange.ValueFilter_continuous,
                          v.petal_length, orange.ValueFilter.Operator.Between, 5)

        self.assertRaises(ValueError, orange.ValueFilter_continuous,
                          v.petal_length, orange.ValueFilter.Operator.Between, min=5)

        self.assertRaises(ValueError, orange.ValueFilter_continuous,
                          v.petal_length, orange.ValueFilter.Operator.Between, max=5)

    def test_valueFilter_discrete(self):
        d = orange.ExampleTable("zoo")
        f = orange.ValueFilter_discrete(d.domain.class_var, values=[2, 3, 4])
        for e in d:
            self.assertEqual(e.getclass() in [2, 3, 4], f(e))

        f.values.variable = d.domain.class_var
        f.values[0] = "mammal"

        f = orange.ValueFilter_discrete(d.domain.class_var, values=(d.domain.class_var, [2, "mammal"]))

        s = pickle.dumps(f)
        f2 = pickle.loads(s)
        self.assertEqual(f2.values.variable, d.domain.class_var)
        self.assertEqual(len(f2.values), 2)
        self.assertIn(2, f2.values)
        self.assertIn(d.domain.class_var.values.index("mammal"), f2.values)

        self.assertRaises(ValueError, orange.ValueFilter_discrete,
                          variable=d.domain.class_var, values=(d.domain.class_var, [2, "martian"]))

        self.assertRaises(TypeError, orange.ValueFilter_discrete,
                          variable=d.domain.class_var, values=(d.domain.class_var, [2, orange.Classifier]))


    def test_valueFilter_string_case_sens(self):
        d = orange.ExampleTable("zoo")
        nameid = d.domain.metaid("name")
        f = orange.ValueFilter_string(d.columns.name,
                                      orange.ValueFilter_string.Operator.Equal,
                                      "girl")
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 1)
        self.assertEqual(d2[0][nameid], "girl")

        f.oper = orange.ValueFilter.Operator.NotEqual
        for e in d:
            self.assertEqual(e[nameid]!="girl", f(e))

        f.oper = orange.ValueFilter.Operator.Less
        for e in d:
            self.assertEqual(e[nameid]<"girl", f(e))

        f.oper = orange.ValueFilter.Operator.LessEqual
        for e in d:
            self.assertEqual(e[nameid]<="girl", f(e))

        f.oper = orange.ValueFilter.Operator.Greater
        for e in d:
            self.assertEqual(e[nameid]>"girl", f(e))

        f.oper = orange.ValueFilter.Operator.GreaterEqual
        for e in d:
            self.assertEqual(e[nameid]>="girl", f(e))

        f.oper = orange.ValueFilter.Operator.Between
        f.max = "lion"
        for e in d:
            self.assertEqual("girl" <= e[nameid] <= "lion", f(e))

        f.oper = orange.ValueFilter.Operator.Outside
        for e in d:
            self.assertEqual("girl" <= e[nameid] <= "lion", not f(e))

        f.oper = orange.ValueFilter_string.Operator.Contains
        f.ref = "ea"
        for e in d:
            self.assertEqual("ea" in str(e[nameid]), f(e))

        f.oper = orange.ValueFilter_string.Operator.BeginsWith
        f.ref = "sea"
        for e in d:
            self.assertEqual(str(e[nameid]).startswith("sea"), f(e))

        f.oper = orange.ValueFilter_string.Operator.EndsWith
        f.ref = "ion"
        for e in d:
            self.assertEqual(str(e[nameid]).endswith("ion"), f(e))


    def test_valueFilter_string_case_insens(self):
        d = orange.ExampleTable("zoo")
        nameid = d.domain.metaid("name")
        f = orange.ValueFilter_string(d.domain["name"],
                                      orange.ValueFilter_string.Operator.Equal,
                                      "GIRL",
                                      case_sensitive=False)
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 1)
        self.assertEqual(d2[0][nameid], "girl")

        f.oper = orange.ValueFilter.Operator.NotEqual
        for e in d:
            self.assertEqual(e[nameid]!="girl", f(e))

        f.oper = orange.ValueFilter.Operator.Less
        for e in d:
            self.assertEqual(e[nameid]<"girl", f(e))

        f.oper = orange.ValueFilter.Operator.LessEqual
        for e in d:
            self.assertEqual(e[nameid]<="girl", f(e))

        f.oper = orange.ValueFilter.Operator.Greater
        for e in d:
            self.assertEqual(e[nameid]>"girl", f(e))

        f.oper = orange.ValueFilter.Operator.GreaterEqual
        for e in d:
            self.assertEqual(e[nameid]>="girl", f(e))

        f.oper = orange.ValueFilter.Operator.Between
        f.max = "LION"
        for e in d:
            self.assertEqual("girl" <= e[nameid] <= "lion", f(e))

        f.oper = orange.ValueFilter.Operator.Outside
        for e in d:
            self.assertEqual("girl" <= e[nameid] <= "lion", not f(e))

        f.oper = orange.ValueFilter_string.Operator.Contains
        f.ref = "EA"
        for e in d:
            self.assertEqual("ea" in str(e[nameid]), f(e))

        f.oper = orange.ValueFilter_string.Operator.NotContains
        f.ref = "EA"
        for e in d:
            self.assertEqual("ea" in str(e[nameid]), not f(e))

        f.oper = orange.ValueFilter_string.Operator.BeginsWith
        f.ref = "SEA"
        for e in d:
            self.assertEqual(str(e[nameid]).startswith("sea"), f(e))

        f.oper = orange.ValueFilter_string.Operator.EndsWith
        f.ref = "ION"
        for e in d:
            self.assertEqual(str(e[nameid]).endswith("ion"), f(e))


    def test_valueFilter_stringList(self):
        d = orange.ExampleTable("zoo")
        v = d.columns
        f = orange.ValueFilter_stringList(variable=v.name, values=["girl"])
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 1)
        self.assertEqual(d2[0][v.name], "girl")

        f.values = ["girl", "hawk", "lion"]
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 3)
        for e in d2:
            self.assertTrue(e["name"] in ["girl", "hawk", "lion"])

        f.negate = True
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), len(d)-3)
        for e in d2:
            self.assertTrue(e["name"] not in ["girl", "hawk", "lion"])

        f.negate= False

        s = pickle.dumps(f)
        f2 = pickle.loads(s)
        d2 = [e for e in d if f2(e)]
        self.assertEqual(len(d2), 3)
        for e in d2:
            self.assertTrue(e[v.name] in ["girl", "hawk", "lion"])

        f.values = ["GIRL", "HAWK", "LION"]
        f.case_sensitive = False
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 3)
        for e in d2:
            self.assertTrue(e["name"], ["girl", "hawk", "lion"])

    def test_values(self):
        d = orange.ExampleTable("zoo")
        v = d.columns
        f = orange.Filter_values()
        f.conditions.append(orange.ValueFilter_discrete(
            variable = v.feathers,
            values=[v.feathers.values.index("0")]))
        f.conditions.append(orange.ValueFilter_discrete(
            v.eggs,
            [d.domain["eggs"].values.index("0")]))
        d2 = [e for e in d if f(e)]
        for e in d2:
            self.assertTrue(e["feathers"]=="0" and e["eggs"]=="0")

        f.conjunction = False
        d2 = [e for e in d if f(e)]
        for e in d2:
            self.assertTrue(e["feathers"]=="0" or e["eggs"]=="0")

        s = pickle.dumps(f)
        f2 = pickle.loads(s)
        d2 = [e for e in d if f2(e)]
        for e in d2:
            self.assertTrue(e["feathers"]=="0" or e["eggs"]=="0")

    def test_sameExample(self):
        d = orange.ExampleTable("iris")
        ex = orange.Example(d.domain, [4.9, 3.1, 1.5, 0.1, "Iris-setosa"])
        f = orange.Filter_sameExample(example=ex)
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 3)

    def test_compatibleExample(self):
        d = orange.ExampleTable("iris")
        ex = orange.Example(d.domain, [4.9, 3.1, 1.5, 0.1, "Iris-setosa"])
        f = orange.Filter_compatibleExample(example=ex)
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 3)

        ex = orange.Example(d.domain, [4.9, "?", "?", "?", "Iris-setosa"])
        f = orange.Filter_compatibleExample(example=ex)
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 4)

        ex = orange.Example(d.domain, [4.9, "?", "?", "?", "?"])
        f = orange.Filter_compatibleExample(example=ex)
        d2 = [e for e in d if f(e)]
        self.assertEqual(len(d2), 6)

    def test_conjunction(self):
        d = orange.ExampleTable("iris")
        f1 = orange.Filter_sameValue(position=0, value=4.9)
        f2 = orange.Filter_sameValue(position=1, value=3.1)
        f = orange.Filter_conjunction()
        f.filters = [f1, f2]
        d2 = [e for e in d if f(e)]
        for e in d2:
            self.assertTrue(e[0]==4.9 and e[1]==3.1)

        f = orange.Filter_conjunction(filters=[f1, f2])
        d2 = [e for e in d if f(e)]
        for e in d2:
            self.assertTrue(e[0]==4.9 and e[1]==3.1)

        f.negate = True
        d2 = [e for e in d if f(e)]
        for e in d2:
            self.assertTrue(not (e[0]==4.9 and e[1]==3.1))

        s = pickle.dumps(f)
        ff = pickle.loads(s)
        d2 = [e for e in d if ff(e)]
        for e in d2:
            self.assertTrue(not (e[0]==4.9 and e[1]==3.1))


    def test_disjunction(self):
        d = orange.ExampleTable("iris")
        f1 = orange.Filter_sameValue(position=0, value=4.9)
        f2 = orange.Filter_sameValue(position=1, value=3.1)
        f = orange.Filter_disjunction()
        f.filters = [f1, f2]
        d2 = [e for e in d if f(e)]
        for e in d2:
            self.assertTrue(e[0]==4.9 or e[1]==3.1)

        f = orange.Filter_disjunction(filters=[f1, f2])
        d2 = [e for e in d if f(e)]
        for e in d2:
            self.assertTrue(e[0]==4.9 or e[1]==3.1)

        f.negate = True
        d2 = [e for e in d if f(e)]
        for e in d2:
            self.assertTrue(not (e[0]==4.9 or e[1]==3.1))

if __name__ == "__main__":
    unittest.main()