import unittest
import orange

class HierarchicalClustering(unittest.TestCase):
    def setUp(self):
        pass

    def rectestlen(self, cluster):
        if cluster.left:
            self.assertEqual(len(cluster), len(cluster.left) + len(cluster.right))
            self.assertEqual(cluster[0], cluster.left[0])
            self.assertEqual(cluster[len(cluster.left)], cluster.right[0])
            self.assertEqual(cluster[-1], cluster.right[-1])
            self.rectestlen(cluster.left)
            self.rectestlen(cluster.right)

    def test_iris(self):
        data = orange.ExampleTable("iris")
        dss = orange.ExamplesDistanceConstructor_Euclidean(data)
        t = orange.HierarchicalClustering.Linkage
        for linkage in [t.Single, t.Average, t.Complete, t.Ward]:
            dist = orange.SymMatrix(len(data))
            for i, e in enumerate(data):
                for j in range(i):
                    dist[i, j] = dss(e, data[j])
            root = orange.HierarchicalClustering(dist, linkage=linkage)
            self.assertEqual(len(root), len(data))
            self.rectestlen(root)
            root.mapping.objects = data
            self.assertEqual(root[0], data[0])


if __name__ == "__main__":
    unittest.main()