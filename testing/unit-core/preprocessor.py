import unittest
import orange

class PreprocessorTestCase(unittest.TestCase):
    def setUp(self):
        self.domain = orange.Domain([orange.ContinuousVariable("a"),
                                     orange.ContinuousVariable("b"),
                                     orange.DiscreteVariable("c", values="abc")])
        self.data = orange.ExampleTable(self.domain)


    def test_removeDuplicates(self):
        self.data += [[0, 0, 0], [0, 0, 1], [0, 1, 0],
                      [1, 0,1], [0, 0, 0], [1, 0, 1]]
        data2 = orange.Preprocessor_removeDuplicates(
            self.data, orange.Preprocessor.Result.Copy)
        self.assertEqual(len(data2), 4)
        self.assertEqual(self.data[0].get_weight(), 1)
        self.assertEqual(data2[0].get_weight(), 2)
        self.assertEqual(data2[1].get_weight(), 1)
        self.assertEqual(data2[2].get_weight(), 1)
        self.assertEqual(data2[3].get_weight(), 2)
        data2[0, 0] = 42
        self.assertEqual(self.data[0, 0], 0)

        data2 = orange.Preprocessor_removeDuplicates(
            self.data, orange.Preprocessor.Result.Reference)
        self.assertEqual(len(data2), 4)
        self.assertEqual(self.data[0].get_weight(), 1)
        self.assertEqual(data2[0].get_weight(), 2)
        self.assertEqual(data2[1].get_weight(), 1)
        self.assertEqual(data2[2].get_weight(), 1)
        self.assertEqual(data2[3].get_weight(), 2)
        self.data[0, 0] = 42
        self.assertEqual(data2[0, 0], 42)
        self.data[0, 0] = 0

        self.data[4].set_weight(2)
        data2 = orange.Preprocessor_removeDuplicates(self.data)
        self.assertEqual(data2[0].get_weight(), 3)
        self.data[4].set_weight(1)

        self.assertRaises(RuntimeError, orange.Preprocessor_removeDuplicates,
            self.data, orange.Preprocessor.Result.InPlace)

        del data2
        data2 = orange.Preprocessor_removeDuplicates(self.data,
            orange.Preprocessor.Result.InPlace)
        self.assertEqual(len(data2), 4)
        self.assertEqual(data2[0].get_weight(), 2)
        self.assertEqual(data2[1].get_weight(), 1)
        self.assertEqual(data2[2].get_weight(), 1)
        self.assertEqual(data2[3].get_weight(), 2)
        self.data[0, 0] = 42
        self.assertEqual(data2[0, 0], 42)


    def test_removeDuplicates_same(self):
        self.data += [[0, 0, 0]]*10
        data2 = orange.Preprocessor_removeDuplicates(
            self.data, orange.Preprocessor.Result.Copy)
        self.assertEqual(len(data2), 1)
        self.assertEqual(data2[0], [0, 0, 0])
        self.assertEqual(data2[0].get_weight(), 10)


    def test_dropMissingValues(self):
        self.data += [[0, 0, 0], [0, 0, "?"], [0, 1, 0],
                      [1, 0,1], [0, "?", 0], [1, 0, 1]]

        data2 = orange.Preprocessor_dropMissing(self.data)
        self.assertEqual(len(data2), 4)
        self.assertIs(self.data, data2.base)


    def test_dropMissingClasses(self):
        self.data += [[0, 0, 0], [0, 0, "?"], [0, 1, 0],
                      [1, 0,1], [0, "?", 0], [1, 0, 1]]

        data2 = orange.Preprocessor_dropMissingClasses(self.data)
        self.assertEqual(len(data2), 5)
        self.assertIs(self.data, data2.base)


    def test_shuffle(self):
        self.data += [[0, 0, 0], [1, 0, "?"], [2, 1, 0],
                      [3, 0,1], [4, "?", 0], [5, 0, 1]]

        r6 = list(range(6))
        for i in range(10):
            data2 = orange.Preprocessor_shuffle(self.data)
            firsts = [e[0] for e in data2]
            if firsts != r6:
                break
        self.assertNotEqual(firsts, r6)
        self.assertSetEqual(set(firsts), set(r6))


    def test_addNoise(self):
        self.data += [[0, 0, 0]]*12 + [[1, 1, 1]]*12 + [[2, 2, 2]]*12

        d2 = orange.Preprocessor_addNoise(self.data)
        cd = len([e for e in d2 if int(e[0])!=int(e[2])])
        self.assertEqual(cd, 0)

        d2 = orange.Preprocessor_addNoise(self.data, proportion=0.25)
        cd = len([e for e in d2 if int(e[0])!=int(e[2])])
        self.assertLessEqual(cd, 9)

        d2 = orange.Preprocessor_addNoise(self.data, deviation = 1)
        cd = len([e for e in d2 if e[0] == 0])
        self.assertLessEqual(cd, 2)


    def test_addMissing(self):
        self.data += [[0, 0, 0]]*12 + [[1, 1, 1]]*12 + [[2, 2, 2]]*12

        d2 = orange.Preprocessor_addMissing(self.data, proportion=0.25)
        cd = len([e for e in d2 if e[0]==None])
        self.assertEqual(cd, 9)
        cd = len([e for e in d2 if e[-1]==None])
        self.assertEqual(cd, 0)

        d2 = orange.Preprocessor_addMissing(self.data,
            proportion=0.25, includeClass=True)
        cd = len([e for e in d2 if e[0]==None])
        self.assertEqual(cd, 9)
        cd = len([e for e in d2 if e[-1]==None])
        self.assertEqual(cd, 9)


    def test_addClassNoise(self):
        self.data += [[0, 0, 0]]*12 + [[1, 1, 1]]*12 + [[2, 2, 2]]*12

        d2 = orange.Preprocessor_addClassNoise(self.data)
        cd = len([e for e in d2 if int(e[0])!=int(e[2])])
        self.assertEqual(cd, 0)

        d2 = orange.Preprocessor_addClassNoise(self.data, proportion=0.25)
        cd = len([e for e in d2 if int(e[0])!=int(e[2])])
        self.assertLessEqual(cd, 9)
        cd = len([e for e in d2 if float(e[0])!=float(e[1])])
        self.assertEqual(cd, 0)


    def test_addClassMissing(self):
        self.data += [[0, 0, 0]]*12 + [[1, 1, 1]]*12 + [[2, 2, 2]]*12

        d2 = orange.Preprocessor_addMissingClasses(self.data, proportion=0.25)
        cd = len([e for e in d2 if e[0]==None])
        self.assertEqual(cd, 0)
        cd = len([e for e in d2 if e[-1]==None])
        self.assertEqual(cd, 9)


    def test_addGaussianClassNoise(self):
        self.assertRaises(
            ValueError, orange.Preprocessor_addGaussianClassNoise, self.data)

        domain = orange.Domain([orange.ContinuousVariable("a"),
                                orange.ContinuousVariable("b")])
        data = orange.ExampleTable(domain)
        data += [[0, 0]]*12 + [[1, 1]]*12 + [[2, 2]]*12
        d2 = orange.Preprocessor_addGaussianClassNoise(data, deviation=1)
        cd = len([e for e in d2 if float(e[0]) == float(e[1])])
        self.assertLessEqual(cd, 2)


    def test_addClassWeight(self):
        self.data += [[0, 0, 0]]*12 + [[1, 1, 1]]*6 + [[2, 2, 2]]*6

        self.assertRaises(ValueError,
            orange.Preprocessor_addClassWeight, self.data)
        self.assertRaises(ValueError,
            orange.Preprocessor_addClassWeight, self.data, classWeights=[1, 2])

        cw = [1, 2, 3]
        d2 = orange.Preprocessor_addClassWeight(self.data, classWeights=cw)
        for e in d2:
            self.assertEqual(e.get_weight(), cw[int(e.getclass())])

        d2 = orange.Preprocessor_addClassWeight(self.data, equalize=True)
        cw = [2/3, 4/3, 4/3]
        for e in d2:
            self.assertAlmostEqual(e.get_weight(), cw[int(e.getclass())])
        for e in orange.get_class_distribution(d2):
            self.assertAlmostEqual(e, 8)

        cw = [1, 2, 3]
        d2 = orange.Preprocessor_addClassWeight(self.data,
            classWeights=cw, equalize=True)
        dd = orange.get_class_distribution(d2)
        self.assertAlmostEqual(dd[0], 8)
        self.assertAlmostEqual(dd[1], 16)
        self.assertAlmostEqual(dd[2], 24)

if __name__ == "__main__":
#    suite = unittest.TestLoader().loadTestsFromName("__main__.ExampleTableTestCase.test_translate_through_slice")
#    unittest.TextTestRunner(verbosity=2).run(suite)
    unittest.main()