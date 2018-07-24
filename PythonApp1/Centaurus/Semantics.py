import ctypes
import multiprocessing as mp
from .CoreLib import CoreLib

class SemanticStore(object):
    def __init__(self, lhs, rhs, rhs_num):
        self.lhs = lhs
        self.rhs = rhs
        self.rhs_num = rhs_num
    def read