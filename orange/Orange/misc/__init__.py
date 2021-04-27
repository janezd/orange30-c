__author__ = 'janezdemsar'

class IdemMap:
    def __init__(self, n): self.n = n

    def __getitem__(self, x): return x
    def __bool__(self):     return False
    def __len__(self):      return self.n
    def __iter__(self):     return range(self.n)