import unittest
import orange

class AssociationRulesTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def testassociationrule(self):
        data = orange.ExampleTable("zoo")
        left = orange.Example(data.domain)
        left["hair"] = "0"
        left["type"] = "mammal"
        right = orange.Example(data.domain)
        right["aquatic"] = "1"
        rule = orange.AssociationRule(left, right)
        self.assertEqual(rule.left, left)
        self.assertEqual(str(rule), "hair=0 type=mammal -> aquatic=1")
        self.assertEqual(rule.support, -1)
        self.assertEqual(rule.confidence, -1)
        for attr in ("coverage", "strength", "lift", "leverage",
                     "n_applies_left", "n_applies_right", "n_examples"):
            self.assertEqual(getattr(rule, attr), 0)
        self.assertEqual(rule.n_left, 2)
        self.assertEqual(rule.n_right, 1)

        rule2 = orange.AssociationRule(rule)
        self.assertEqual(rule, rule2)

        self.assertTrue(rule.applies_left(left))
        self.assertFalse(rule.applies_left(right))
        self.assertFalse(rule.applies_right(left))
        self.assertTrue(rule.applies_right(right))
        self.assertFalse(rule.applies_both(left))
        self.assertFalse(rule.applies_both(right))

        both = orange.Example(left)
        both["aquatic"] = "1"
        self.assertTrue(rule.applies_left(both))
        self.assertTrue(rule.applies_right(both))
        self.assertTrue(rule.applies_both(both))

        import pickle
        s = pickle.dumps(rule)
        rule3 = pickle.loads(s)
        self.assertEqual(rule, rule3)
        rule = rule3
        self.assertEqual(rule.left, left)
        self.assertEqual(str(rule), "hair=0 type=mammal -> aquatic=1")
        self.assertEqual(rule.support, -1)
        self.assertEqual(rule.confidence, -1)
        for attr in ("coverage", "strength", "lift", "leverage",
                     "n_applies_left", "n_applies_right", "n_examples"):
            self.assertEqual(getattr(rule, attr), 0)
        self.assertEqual(rule.n_left, 2)
        self.assertEqual(rule.n_right, 1)

    def testinducer(self):
        # A good example of very poorly written and uninspired unit test
        # Regression tests should take care of this
        data = orange.ExampleTable("zoo")
        rules = orange.AssociationRulesInducer(data, support=0.5, confidence=0.9)
        rules2 = orange.AssociationRulesInducer(support=0.5, confidence=0.9)(data)
        self.assertEqual(rules, rules2)

        data = orange.ExampleTable("iris")
        self.assertRaises(TypeError, orange.AssociationRulesInducer, data)

if __name__ == "__main__":
    unittest.main()