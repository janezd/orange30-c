import unittest
import orange
import math
import pickle

class DiscretizationTest(unittest.TestCase):
    def setUp(self):
        pass

    def test_equalWidth(self):
        d = orange.ExampleTable("iris")
        ba = orange.DomainBasicAttrStat(d)
        ddisc = orange.DomainDiscretization(orange.EqualWidthDiscretization())
        dd = ddisc(d)
        for i in range(4):
            self.assertEqual(len(dd[i].values), 4)
            mi, ma = ba[i].min, ba[i].max
            di = ma-mi
            trans = dd[i].get_value_from.transformer
            self.assertAlmostEqual(trans.first_cut, mi+di/4, 1)
            self.assertAlmostEqual(trans.step, di/4, 1)
            self.assertEqual(trans.n_intervals, 4)

        ddisc.discretization.n_intervals = 5

        dd = ddisc(d)
        for i in range(4):
            self.assertEqual(len(dd[i].values), 5)
            mi, ma = ba[i].min, ba[i].max
            di = ma-mi
            trans = dd[i].get_value_from.transformer
            self.assertAlmostEqual(trans.first_cut, mi+di/5, 1)
            self.assertAlmostEqual(trans.step, di/5, 1)
            self.assertEqual(trans.n_intervals, 5)
            points = trans.points
            for j in range(4):
                self.assertAlmostEqual(trans.points[i], trans.first_cut + i*di/5)

        d2 = orange.ExampleTable(dd, d)
        for e, e2 in zip(d[:5], d2):
            for i in range(4):
                trans = dd[i].get_value_from.transformer
                self.assertEqual(e2[i], math.floor((e[i]-trans.firstCut) / trans.step) + 1)

        s = pickle.dumps(dd)
        dd2 = pickle.loads(s)
        d3 = orange.ExampleTable(dd2, d)
        for e, e2 in zip(d[:5], d3):
            for i in range(4):
                trans = dd[i].get_value_from.transformer
                self.assertEqual(e2[i], math.floor((e[i]-trans.firstCut) / trans.step) + 1)
                
    def test_equalFreq(self):
        d = orange.ExampleTable("iris")
        for i in range(150):
            d[i, 0] = i
        dd = orange.DomainDiscretization(orange.EqualFreqDiscretization(n_intervals=5), d)
        d2 = orange.ExampleTable(dd, d)
        dist = orange.DomainDistributions(d2)
        self.assertEqual(dist[0], [30]*5)
        self.assertEqual(dd[0].get_value_from.transformer.points, [29.5, 59.5, 89.5, 119.5])

        v2 = orange.EqualFreqDiscretization(n_intervals=5)(d.domain[0], d)
        self.assertEqual(v2.get_value_from.transformer.points, [29.5, 59.5, 89.5, 119.5])

        s = pickle.dumps(dd)
        dd2 = pickle.loads(s)
        self.assertEqual(dd2[0].get_value_from.transformer.points, [29.5, 59.5, 89.5, 119.5])

if __name__ == "__main__":
    unittest.main()