import unittest
import orange

# This script has three test cases: they test IntList, StringList and VarList
# IntList has a "primitive" C type, StringList's elements are C++ objects (with
# which we had problems with realloc) and VarList contains wrapped objects.
# I guess this covers all cases; if lists work for these three types, they will
# work for any other type, too

class IntListTestCase(unittest.TestCase):
    def setUp(self):
        self.l = orange.IntList(range(10))

    def test_len(self):
        self.assertEqual(len(self.l), 10)
        
    def test_empty(self):
        l = orange.IntList()
        self.assertEqual(len(l), 0)
        l = orange.IntList([])
        self.assertEqual(len(l), 0)

    def test_wrongType(self):
        self.assertRaises(TypeError, orange.IntList, "adf")
        self.assertRaises(TypeError, orange.IntList, (["adf"]))

    def test_item(self):
        self.assertEqual(self.l[0], 0)
        self.assertEqual(self.l[2], 2)
        self.assertEqual(self.l[9], 9)
        self.assertEqual(self.l[-1], 9)
        self.assertEqual(self.l[-2], 8)
        self.assertEqual(self.l[-10], 0)
        with self.assertRaises(IndexError):
            self.l[10]
        with self.assertRaises(IndexError):
            self.l[-11]

    def test_ass_item(self):
        for i in range(10):
            self.l[i] = 42+i
            self.assertEqual(self.l[i], 42+i)
            self.l[-i-1] = 24+i
            self.assertEqual(self.l[-i-1], 24+i)

    def test_slice_and_cmp(self):
        self.assertEqual(self.l[2:5], orange.IntList(range(2, 5)))
        self.assertEqual(self.l[2:5], range(2, 5))
        self.assertGreater(self.l[2:5], orange.IntList(range(1, 5)))
        self.assertGreater(self.l[2:5], orange.IntList([1]))
        self.assertGreater(self.l[2:5], orange.IntList())
        self.assertGreater(self.l[2:5], [])
        self.assertGreater(self.l[2:5], [])
        self.assertLess([], self.l[2:5])
        self.assertLess([1, 2, 3], self.l[2:5])
        self.assertEqual(self.l[::-1], range(9, -1, -1))

    def test_ass_slice(self):
        self.l[2:5] = [12, 13, 14]
        self.assertEqual(self.l, [0, 1, 12, 13, 14, 5, 6, 7, 8, 9])
        self.l[2:5] = [22]
        self.assertEqual(self.l, [0, 1, 22, 5, 6, 7, 8, 9])
        self.l[2:3] = [23, 24, 25]
        self.assertEqual(self.l, [0, 1, 23, 24, 25, 5, 6, 7, 8, 9])
        self.l[2:5] = []
        self.assertEqual(self.l, [0, 1, 5, 6, 7, 8, 9])
        del self.l[:3]
        self.assertEqual(self.l, [6, 7, 8, 9])
        del self.l[:]
        self.assertEqual(len(self.l), 0)
        self.l[:]=range(5)
        self.assertEqual(self.l, range(5))
        
        
    def test_in(self):
        i = 2 in self.l
        self.assertTrue(i)
        i = 11 in self.l
        self.assertFalse(i)
        with self.assertRaises(TypeError):
            "a" in self.l

    def test_repeat(self):
        self.assertEqual(self.l*3, list(range(10))*3)
        self.assertEqual(orange.IntList([])*3, [])

    def test_concat(self):
        first = orange.IntList(range(5))
        second = orange.IntList(range(5, 10))
        empty = orange.IntList()
        self.assertEqual(first+second, self.l)
        self.assertEqual(first+[], first)
        self.assertEqual(first+empty, first)
        self.assertEqual(empty+first, first)
        self.assertEqual(empty+empty, empty)

    def test_append(self):
        self.l.append(10)
        self.assertEqual(self.l, range(11))
        with self.assertRaises(TypeError):
            self.l.append("x")

    def test_count(self):
        self.assertEqual(self.l.count(3), 1)
        self.assertEqual(self.l.count(13), 0)
        self.assertRaises(TypeError, self.l.count, "x")
        self.assertEqual((self.l*3).count(3), 3)
        
    def test_index(self):
        self.assertEqual(self.l.index(3), 3)
        self.assertRaises(ValueError, self.l.index, 13)
        self.assertRaises(TypeError, self.l.index, "x")

    def test_insert(self):
        r = list(range(10))
        self.l.insert(0, 10)
        r.insert(0, 10)
        self.assertEqual(self.l, r)
        self.l.insert(2, 11)
        r.insert(2, 11)
        self.assertEqual(self.l, r)
        r.insert(12, 12)
        self.l.insert(12, 12)
        self.assertEqual(self.l, r)
        self.l.insert(-2, 13)
        r.insert(-2, 13)
        self.assertEqual(self.l, r)
        self.l.insert(-20, 14)
        r.insert(-20, 14)
        self.assertEqual(self.l, r)
        self.l.insert(20, 15)
        r.insert(20, 15)
        self.assertEqual(self.l, r)
        empty = orange.IntList()
        empty.insert(0, 0)
        self.assertEqual(empty, [0])
        empty = orange.IntList()
        empty.insert(10, 0)
        self.assertEqual(empty, [0])
        empty = orange.IntList()
        empty.insert(-10, 0)
        self.assertEqual(empty, [0])

    def test_remove(self):
        self.l.remove(0)
        self.assertEqual(self.l, range(1, 10))
        self.l.remove(9)
        self.assertEqual(self.l, range(1, 9))
        self.l.remove(5)
        self.assertEqual(self.l, [1, 2, 3, 4, 6, 7, 8])
        self.assertRaises(ValueError, self.l.remove, 42)
        self.assertRaises(TypeError, self.l.remove, "x")

    def test_reverse(self):
        self.l.reverse()
        self.assertEqual(self.l, range(9, -1, -1))
        empty = orange.IntList()
        empty.reverse()
        self.assertEqual(empty, [])
        self.assertFalse(empty)

    def test_sort(self):
        import random
        r = [random.randint(1, 100) for i in range(20)]
        l = orange.IntList(r)
        l.sort()
        r.sort()
        self.assertEqual(l, r)

    def test_pop(self):
        self.assertEqual(self.l.pop(), 9)
        self.assertEqual(self.l, range(9))
        self.l.pop(1)
        self.assertEqual(self.l, [0, 2, 3, 4, 5, 6, 7, 8])
        self.l.pop(0)
        self.assertEqual(self.l, [2, 3, 4, 5, 6, 7, 8])
        l = orange.IntList([1])
        self.assertEqual(l.pop(), 1)
        self.assertFalse(l)
        with self.assertRaises(IndexError):
            l.pop()



class StringListTestCase(unittest.TestCase):
    def setUp(self):
        self.l = orange.StringList("abcde")

    def test_len(self):
        self.assertEqual(len(self.l), 5)
        
    def test_empty(self):
        l = orange.StringList()
        self.assertEqual(len(l), 0)
        l = orange.StringList([])
        self.assertEqual(len(l), 0)

    def test_wrongType(self):
        self.assertRaises(TypeError, orange.StringList, 42)
        self.assertRaises(TypeError, orange.StringList, ([42]))

    def test_item(self):
        self.assertEqual(self.l[0], "a")
        self.assertEqual(self.l[2], "c")
        self.assertEqual(self.l[-1], "e")
        self.assertEqual(self.l[-2], "d")
        with self.assertRaises(IndexError):
            self.l[10]
        with self.assertRaises(IndexError):
            self.l[-11]

    def test_ass_item(self):
        self.l[2] = "x"
        self.assertEqual(self.l, "abxde")

    def test_slice_and_cmp(self):
        self.assertEqual(self.l[1:3], "bc")
        self.assertGreater(self.l[1:3], "ab")
        self.assertGreater(self.l[2:5], orange.StringList("ab"))
        self.assertGreater(self.l[2:5], orange.StringList())
        with self.assertRaises(TypeError):
            self.assertGreater(self.l[2:5], orange.IntList([1, 2]))

    def test_ass_slice(self):
        self.l[1:3] = "BC"
        self.assertEqual(self.l, "aBCde")
        self.l[1:3] = []
        self.assertEqual(self.l, "ade")
        del self.l[:2]
        self.assertEqual(self.l, "e")
        del self.l[:]
        self.assertEqual(len(self.l), 0)
        self.l[:] = "abc"
        self.assertEqual(self.l, "abc")
        
        
    def test_in(self):
        i = "a" in self.l
        self.assertTrue(i)
        i = "X" in self.l
        self.assertFalse(i)
        with self.assertRaises(TypeError):
            42 in self.l

    def test_repeat(self):
        self.assertEqual(self.l*3, "abcde"*3)
        self.assertEqual(orange.StringList([])*3, [])

    def test_concat(self):
        first = orange.StringList("abcde")
        second = orange.StringList("abcde")
        empty = orange.StringList()
        self.assertEqual(first+second, "abcdeabcde")
        self.assertEqual(first+first, "abcdeabcde")
        self.assertEqual(first+"", "abcde")

    def test_append(self):
        self.l.append("f")
        self.assertEqual(self.l, "abcdef")
        with self.assertRaises(TypeError):
            self.l.append(12)

    def test_count(self):
        self.assertEqual(self.l.count("a"), 1)
        self.assertEqual(self.l.count("x"), 0)
        self.assertRaises(TypeError, self.l.count, 42)
        self.assertEqual((self.l*3).count("a"), 3)
        
    def test_index(self):
        self.assertEqual(self.l.index("c"), 2)
        self.assertRaises(ValueError, self.l.index, "x")
        self.assertRaises(TypeError, self.l.index, 13)

    def test_insert(self):
        r = list("abcde")
        self.l.insert(0, "x")
        r.insert(0, "x")
        self.assertEqual(self.l, r)
        self.l.insert(2, "y")
        r.insert(2, "y")
        self.assertEqual(self.l, r)
        r.insert(7, "z")
        self.l.insert(7, "z")
        self.assertEqual(self.l, r)
        self.l.insert(-2, "u")
        r.insert(-2, "u")
        self.assertEqual(self.l, r)
        self.l.insert(-20, "v")
        r.insert(-20, "v")
        self.assertEqual(self.l, r)
        self.l.insert(20, "w")
        r.insert(20, "w")
        self.assertEqual(self.l, r)
        empty = orange.StringList()
        empty.insert(0, "a")
        self.assertEqual(empty, "a")
        empty = orange.StringList()
        empty.insert(10, "a")
        self.assertEqual(empty, "a")
        empty = orange.StringList()
        empty.insert(-10, "a")
        self.assertEqual(empty, "a")

    def test_remove(self):
        self.l.remove("a")
        self.assertEqual(self.l, "bcde")
        self.l.remove("e")
        self.assertEqual(self.l, "bcd")
        self.l.remove("c")
        self.assertEqual(self.l, "bd")
        self.assertRaises(ValueError, self.l.remove, "x")
        self.assertRaises(TypeError, self.l.remove, 42)

    def test_reverse(self):
        self.l.reverse()
        self.assertEqual(self.l, "edcba")
        empty = orange.StringList()
        empty.reverse()
        self.assertEqual(empty, [])
        self.assertFalse(empty)

    def test_sort(self):
        import random
        r = [chr(random.randint(65, 90)) for i in range(20)]
        l = orange.StringList(r)
        l.sort()
        r.sort()
        self.assertEqual(l, r)

    def test_pop(self):
        self.assertEqual(self.l.pop(), "e")
        self.assertEqual(self.l, "abcd")
        l = orange.StringList("a")
        self.assertEqual(l.pop(), "a")
        self.assertFalse(l)
        with self.assertRaises(IndexError):
            l.pop()

        

def tv(s):
    return [orange.ContinuousVariable(x) for x in s]

class VarListTestCase(unittest.TestCase):
    def setUp(self):
        self.ll = tv("abcde")
        self.l = orange.VarList(self.ll)

    def test_len(self):
        self.assertEqual(len(self.l), 5)
        
    def test_empty(self):
        l = orange.VarList()
        self.assertEqual(len(l), 0)
        l = orange.VarList([])
        self.assertEqual(len(l), 0)

    def test_wrongType(self):
        self.assertRaises(TypeError, orange.VarList, 42)
        self.assertRaises(TypeError, orange.VarList, ([42]))

    def test_item(self):
        self.assertEqual(self.l[0], self.ll[0])
        self.assertEqual(self.l[2], self.ll[2])
        self.assertEqual(self.l[-1], self.ll[-1])
        self.assertEqual(self.l[-2], self.ll[-2])
        with self.assertRaises(IndexError):
            self.l[10]
        with self.assertRaises(IndexError):
            self.l[-11]

    def test_ass_item(self):
        self.l[2] = self.ll[2] = orange.ContinuousVariable("x")
        self.assertEqual(self.l, self.ll)

    def test_slice(self):
        self.assertEqual(self.l[1:3], self.ll[1:3])

    def test_ass_slice(self):
        lli = tv("BC")
        self.ll[1:3] = lli
        self.l[1:3] = lli
        self.assertEqual(self.l, self.ll)
        self.ll[1:3] = []
        self.l[1:3] = []
        self.assertEqual(self.l, self.ll)
        del self.ll[:2]
        del self.l[:2]
        self.assertEqual(self.l, self.ll)
        del self.l[:]
        self.assertEqual(len(self.l), 0)
        self.l[:] = self.ll[:] = tv("abc")
        self.assertEqual(self.l, self.ll)
        
        
    def test_in(self):
        i = self.ll[1] in self.l
        self.assertTrue(i)
        i = orange.ContinuousVariable("a") in self.l
        self.assertFalse(i)
        with self.assertRaises(TypeError):
            42 in self.l

    def test_repeat(self):
        self.assertEqual(self.l*3, self.ll*3)
        self.assertEqual(orange.VarList([])*3, [])

    def test_append(self):
        f = orange.ContinuousVariable("f")
        self.ll.append(f)
        self.l.append(f)
        self.assertEqual(self.l, self.ll)
        with self.assertRaises(TypeError):
            self.l.append(12)

    def test_count(self):
        self.assertEqual(self.l.count(self.ll[1]), 1)
        self.assertEqual(self.l.count(orange.ContinuousVariable()), 0)
        self.assertRaises(TypeError, self.l.count, 42)
        self.assertEqual((self.l*3).count(self.ll[1]), 3)
        
    def test_index(self):
        self.assertEqual(self.l.index(self.ll[2]), 2)
        self.assertRaises(ValueError, self.l.index, orange.ContinuousVariable())
        self.assertRaises(TypeError, self.l.index, 13)

    def test_insert(self):
        f = orange.ContinuousVariable("f")
        self.ll.insert(2, f)
        self.l.insert(2, f)
        self.assertEqual(self.l, self.ll)

    def test_remove(self):
        self.l.remove(self.ll[2])
        self.ll.remove(self.ll[2])
        self.assertEqual(self.l, self.ll)
        self.assertRaises(ValueError, self.l.remove, orange.ContinuousVariable())
        self.assertRaises(TypeError, self.l.remove, 42)

    def test_reverse(self):
        self.l.reverse()
        self.ll.reverse()
        self.assertEqual(self.l, self.ll)
        empty = orange.VarList()
        empty.reverse()
        self.assertEqual(empty, [])
        self.assertFalse(empty)

    def test_pop(self):
        self.assertEqual(self.l.pop(), self.ll.pop())
        self.assertEqual(self.l, self.ll)
        ll = tv("a")
        l = orange.VarList(ll)
        self.assertEqual(l.pop(), ll.pop())
        self.assertFalse(l, ll)
        with self.assertRaises(IndexError):
            l.pop()

        
if __name__ == "__main__":
    unittest.main()