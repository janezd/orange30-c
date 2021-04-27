import unittest
import orange

class ScoreFeatureTestCase(unittest.TestCase):
    def setUp(self):
        self.domain = orange.Domain([
            orange.DiscreteVariable("corr2", values="01"),
            orange.DiscreteVariable("corr2_h", values="01"),
            orange.DiscreteVariable("corr4", values="0123"),
            orange.DiscreteVariable("uncorr", values="01"),
            orange.DiscreteVariable("const1", values="0"),
            orange.DiscreteVariable("const2", values="01"),
            orange.DiscreteVariable("unk_disc", values="01"),
            orange.ContinuousVariable("const"),
            orange.ContinuousVariable("thresh"),
            orange.ContinuousVariable("thresh_h"),
            orange.ContinuousVariable("bin_corr"),
            orange.ContinuousVariable("bin_uncorr"),
            orange.ContinuousVariable("unk_cont"),
            orange.DiscreteVariable("y", values="01")])
        self.art = orange.ExampleTable(self.domain)
        for i in range(24):
            self.art.append([i//12, i//12 if i%2 else "?", i//6, i%2, 0, 0, None,
                             42, i, i if i%2 else "?", i//12, i%2, None,
                             i//12])

    def test_info(self):
        sf = orange.ScoreFeature_info()

        self.assertAlmostEqual(sf("corr2", self.art), 1)
        self.assertAlmostEqual(sf("corr2_h", self.art), 0.5)
        self.assertAlmostEqual(sf("corr4", self.art), 1)
        self.assertAlmostEqual(sf("uncorr", self.art), 0)
        self.assertAlmostEqual(sf("const1", self.art), 0)
        self.assertAlmostEqual(sf("const2", self.art), 0)
        self.assertAlmostEqual(sf("unk_disc", self.art), orange.ScoreFeature.Rejected)
        self.assertRaises(TypeError, sf, "thresh", self.art)

        thresh, score, dist = sf.best_threshold("const", self.art)
        self.assertAlmostEqual(thresh, -1)
        self.assertAlmostEqual(score, orange.ScoreFeature.Rejected)

        thresh, score, dist = sf.best_threshold("thresh", self.art)
        self.assertAlmostEqual(thresh, 11.5)
        self.assertAlmostEqual(score, 1)

        thresh, score, dist = sf.best_threshold("thresh_h", self.art)
        self.assertAlmostEqual(thresh, 12)
        self.assertAlmostEqual(score, 0.5)

        thresh, score, dist = sf.best_threshold("bin_corr", self.art)
        self.assertAlmostEqual(thresh, 0.5)
        self.assertAlmostEqual(score, 1)

        thresh, score, dist = sf.best_threshold("bin_uncorr", self.art)
        self.assertAlmostEqual(thresh, 0.5)
        self.assertAlmostEqual(score, 0)

        thresh, score, dist = sf.best_threshold("unk_cont", self.art)
        self.assertAlmostEqual(thresh, -1)
        self.assertAlmostEqual(score, orange.ScoreFeature.Rejected)

        threshs = sf.threshold_function("const", self.art)
        self.assertEqual(len(threshs), 0)

        threshs = sf.threshold_function("thresh", self.art)
        bestt, bests = max(threshs, key=lambda t:t[1])
        self.assertAlmostEqual(bestt, 11.5)
        self.assertAlmostEqual(bests, 1)
        self.assertAlmostEqual(threshs[0][0], 0.5)

        threshs = sf.threshold_function("thresh_h", self.art)
        bestt, bests = max(threshs, key=lambda t:t[1])
        self.assertAlmostEqual(bestt, 12)
        self.assertAlmostEqual(bests, 0.5)

        threshs = sf.threshold_function("thresh_h", self.art)
        bestt, bests = max(threshs, key=lambda t:t[1])
        self.assertAlmostEqual(bestt, 12)
        self.assertAlmostEqual(bests, 0.5)


    def test_ratio(self):
        sf = orange.ScoreFeature_gainRatio()
        self.assertAlmostEqual(sf("corr2", self.art), 1)
        self.assertAlmostEqual(sf("corr2_h", self.art), 0.5)
        self.assertAlmostEqual(sf("corr4", self.art), 0.5)
        self.assertAlmostEqual(sf("uncorr", self.art), 0)
        self.assertAlmostEqual(sf("const1", self.art), 0)
        self.assertAlmostEqual(sf("const2", self.art), 0)
        self.assertAlmostEqual(sf("unk_disc", self.art), orange.ScoreFeature.Rejected)
        self.assertRaises(TypeError, sf, "thresh", self.art)

        thresh, score, dist = sf.best_threshold("const", self.art)
        self.assertAlmostEqual(thresh, -1)
        self.assertAlmostEqual(score, orange.ScoreFeature.Rejected)

        thresh, score, dist = sf.best_threshold("thresh", self.art)
        self.assertAlmostEqual(thresh, 11.5)
        self.assertAlmostEqual(score, 1)

        thresh, score, dist = sf.best_threshold("thresh_h", self.art)
        self.assertAlmostEqual(thresh, 12)
        self.assertAlmostEqual(score, 0.5)

        thresh, score, dist = sf.best_threshold("bin_corr", self.art)
        self.assertAlmostEqual(thresh, 0.5)
        self.assertAlmostEqual(score, 1)

        thresh, score, dist = sf.best_threshold("bin_uncorr", self.art)
        self.assertAlmostEqual(thresh, 0.5)
        self.assertAlmostEqual(score, 0)

        thresh, score, dist = sf.best_threshold("unk_cont", self.art)
        self.assertAlmostEqual(thresh, -1)
        self.assertAlmostEqual(score, orange.ScoreFeature.Rejected)


    def test_gini(self):
        sf = orange.ScoreFeature_gini()
        self.assertAlmostEqual(sf("corr2", self.art), 0.25)
        self.assertAlmostEqual(sf("corr2_h", self.art), 0.125)
        self.assertAlmostEqual(sf("corr4", self.art), 0.25)
        self.assertAlmostEqual(sf("uncorr", self.art), 0)
        self.assertAlmostEqual(sf("const1", self.art), 0)
        self.assertAlmostEqual(sf("const2", self.art), 0)
        self.assertAlmostEqual(sf("unk_disc", self.art), orange.ScoreFeature.Rejected)
        self.assertRaises(TypeError, sf, "thresh", self.art)

        thresh, score, dist = sf.best_threshold("const", self.art)
        self.assertAlmostEqual(thresh, -1)
        self.assertAlmostEqual(score, orange.ScoreFeature.Rejected)

        thresh, score, dist = sf.best_threshold("thresh", self.art)
        self.assertAlmostEqual(thresh, 11.5)
        self.assertAlmostEqual(score, 0.25)

        thresh, score, dist = sf.best_threshold("thresh_h", self.art)
        self.assertAlmostEqual(thresh, 12)
        self.assertAlmostEqual(score, 0.125)

        thresh, score, dist = sf.best_threshold("bin_corr", self.art)
        self.assertAlmostEqual(thresh, 0.5)
        self.assertAlmostEqual(score, 0.25)

        thresh, score, dist = sf.best_threshold("bin_uncorr", self.art)
        self.assertAlmostEqual(thresh, 0.5)
        self.assertAlmostEqual(score, 0)

        thresh, score, dist = sf.best_threshold("unk_cont", self.art)
        self.assertAlmostEqual(thresh, -1)
        self.assertAlmostEqual(score, orange.ScoreFeature.Rejected)

    def test_relief(self):
        sf = orange.ScoreFeature_relief()
        sf.m = -1
        sf.k = 2

        self.assertAlmostEqual(sf("corr2", self.art), 1)
        self.assertAlmostEqual(sf("corr2_h", self.art), 0.5)
        self.assertAlmostEqual(sf("corr4", self.art), 1)
        self.assertAlmostEqual(sf("uncorr", self.art), 0)
        self.assertAlmostEqual(sf("const1", self.art), 0)
        self.assertAlmostEqual(sf("const2", self.art), 0)
        self.assertAlmostEqual(sf("unk_disc", self.art), orange.ScoreFeature.Rejected)
        sf("thresh", self.art)
        thresh, score, dist = sf.best_threshold("const", self.art)
        self.assertAlmostEqual(thresh, 42)
        self.assertAlmostEqual(score, orange.ScoreFeature.Rejected)

        thresh, score, dist = sf.best_threshold("thresh", self.art)
        self.assertAlmostEqual(thresh, 11.5)

        thresh, score, dist = sf.best_threshold("thresh_h", self.art)
        self.assertAlmostEqual(thresh, 12)

        thresh, score, dist = sf.best_threshold("bin_corr", self.art)
        self.assertAlmostEqual(thresh, 0.5)
        self.assertAlmostEqual(score, 1)

        thresh, score, dist = sf.best_threshold("bin_uncorr", self.art)
        self.assertTrue(thresh in [0.5, 1])
        self.assertAlmostEqual(score, 0)

        thresh, score, dist = sf.best_threshold("unk_cont", self.art)
        self.assertAlmostEqual(thresh, -1)
        self.assertAlmostEqual(score, orange.ScoreFeature.Rejected)



if __name__ == "__main__":
    unittest.main()
