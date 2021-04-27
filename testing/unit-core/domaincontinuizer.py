import pickle
import unittest
import orange

class DomainContinuizer(unittest.TestCase):
    def setUp(self):
        pass

    def test_continuizer_zoo(self):
        d = orange.ExampleTable("zoo")
        dd = orange.DomainDistributions(d)
        for i, e in enumerate(dd):
            if i==2:
                break

        dc = orange.DomainContinuizer()

        dc.multinomial_treatment = dc.MultinomialTreatment.LowestIsBase

        dc.class_treatment = dc.ClassTreatment.ErrorIfCannotHandle
        self.assertRaises(ValueError, dc, d.domain)

        dc.class_treatment = dc.ClassTreatment.LeaveUnlessTarget
        cdomain = dc(d.domain)
        dd = orange.ExampleTable(cdomain, d)
        self.assertEqual(list(map(int, d[0]))[:3], list(map(int, dd[0]))[:3])
        for l in [2, 4, 5, 6, 8]:
            self.assertEqual(int(dd[0, "legs=%i" % l]), l==4)
        self.assertFalse("legs=0" in cdomain)
        self.assertEqual(cdomain.classVar.name, "type")
        self.assertFalse(cdomain.has_discrete_attributes())
        self.assertFalse(cdomain.has_discrete_attributes(False))
        self.assertTrue(cdomain.has_discrete_attributes(True))

        dc.class_treatment = dc.ClassTreatment.AsOrdinal
        cdomain = dc(d.domain)
        dd = orange.ExampleTable(cdomain, d)
        self.assertEqual(list(map(int, d[0]))[:3], list(map(int, dd[0]))[:3])
        for l in [2, 4, 5, 6, 8]:
            self.assertEqual(int(dd[0, "legs=%i" % l]), l==4)
        self.assertFalse("legs=0" in cdomain)
        self.assertEqual(cdomain.classVar.name, "C_type")
        self.assertEqual(dd[0, -1], d.domain.class_var.values.index("mammal"))
        self.assertFalse(cdomain.has_discrete_attributes())

        dc.class_treatment = dc.ClassTreatment.AsOrdinal
        cdomain = dc(d)
        dd = orange.ExampleTable(cdomain, d)
        self.assertEqual(list(map(int, d[0]))[:3], list(map(int, dd[0]))[:3])
        for l in [2, 4, 5, 6, 8]:
            self.assertEqual(int(dd[0, "legs=%i" % l]), l==4)
        self.assertFalse("legs=0" in cdomain)
        self.assertEqual(cdomain.classVar.name, "C_type")
        self.assertEqual(dd[0, -1], d.domain.class_var.values.index("mammal"))
        self.assertFalse(cdomain.has_discrete_attributes())

        dc.multinomial_treatment = dc.MultinomialTreatment.FrequentIsBase
        self.assertRaises(ValueError, dc, d.domain)
        cdomain = dc(d)
        dd = orange.ExampleTable(cdomain, d)
        self.assertEqual(dd[0, 0], 1)
        self.assertEqual(dd[0, 1], 0)
        self.assertEqual(dd[0, 2], 1)

        dc.multinomial_treatment = dc.MultinomialTreatment.FrequentIsBase
        dc.zero_based = False
        self.assertRaises(ValueError, dc, d.domain)
        cdomain = dc(d)
        dd = orange.ExampleTable(cdomain, d)
        self.assertEqual(dd[0, 0], 1)
        self.assertEqual(dd[0, 1], -1)
        self.assertEqual(dd[0, 2], 1)
        dc.zero_based = True

        dc.multinomial_treatment = dc.MultinomialTreatment.NValues
        cdomain = dc(d.domain)
        dd = orange.ExampleTable(cdomain, d)
        for l in [0, 2, 4, 5, 6, 8]:
            self.assertEqual(int(dd[0, "legs=%i" % l]), l==4)

        dc.multinomial_treatment = dc.MultinomialTreatment.Ignore
        cdomain = dc(d.domain)
        for l in [0, 2, 4, 5, 6, 8]:
            self.assertFalse("legs=%i" in cdomain)

        dc.multinomial_treatment = dc.MultinomialTreatment.IgnoreAllDiscrete
        cdomain = dc(d.domain)
        self.assertEqual(cdomain.variables, [cdomain.class_var])

        dc.multinomial_treatment = dc.MultinomialTreatment.ReportError
        self.assertRaises(ValueError, dc, d.domain)

        dc.multinomial_treatment = dc.MultinomialTreatment.AsOrdinal
        cdomain = dc(d.domain)
        dd = orange.ExampleTable(cdomain, d)
        for e, ec in zip(d[:10], dd):
            self.assertEqual(int(e["legs"]), ec["C_legs"])

        dc.multinomial_treatment = dc.MultinomialTreatment.AsNormalizedOrdinal
        cdomain = dc(d.domain)
        dd = orange.ExampleTable(cdomain, d)
        for e, ec in zip(d[:10], dd):
            self.assertEqual(int(e["legs"])/5, ec["C_legs"])

    def test_continuizer_iris(self):
        d = orange.ExampleTable("iris")
        dc = orange.DomainContinuizer()
        dc.class_treatment = dc.ClassTreatment.LeaveUnlessTarget
        dc.continuous_treatment = dc.ContinuousTreatment.Leave
        cdomain = dc(d.domain)
        self.assertEqual(cdomain.variables, d.domain.variables)
        
        dc.continuous_treatment = dc.ContinuousTreatment.NormalizeBySpan
        self.assertRaises(ValueError, dc, d.domain)
        cdomain = dc(d)
        dd = orange.ExampleTable(cdomain, d)
        bs = orange.DomainBasicAttrStat(d)
        for e, ec in zip(d[:10], dd):
            for i in range(4):
                self.assertEqual((e[i]-bs[i].min) / (bs[i].max - bs[i].min), ec[i])

        dc.continuous_treatment = dc.ContinuousTreatment.NormalizeByVariance
        self.assertRaises(ValueError, dc, d.domain)
        cdomain = dc(d)
        dd = orange.ExampleTable(cdomain, d)
        bs = orange.DomainBasicAttrStat(d)
        for e, ec in zip(d[:10], dd):
            for i in range(4):
                self.assertEqual((e[i]-bs[i].avg) / bs[i].dev, ec[i])


if __name__ == "__main__":
#    suite = unittest.TestLoader().loadTestsFromName("__main__.ExampleTableTestCase.test_translate_through_slice")
#    unittest.TextTestRunner(verbosity=2).run(suite)
    unittest.main()