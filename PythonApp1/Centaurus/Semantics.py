import logging
import ctypes
from .CoreLib import CoreLib

class SymbolEntry(ctypes.Structure):
    _fields_ = [('id', ctypes.c_int),
                ('key', ctypes.c_int),
                ('start', ctypes.c_long),
                ('end', ctypes.c_long)]

class SemanticStore(object):
    def __init__(self, grammar, window, symbols, num):
        self.grammar = grammar
        self.window = window
        self.symbols = symbols
        self.num = num
    def read(self):
        start_addr = self.window + self.symbols[0].start
        end_addr = self.window + self.symbols[0].end
        return ctypes.string_at(ctypes.c_void_p(start_addr), end_addr - start_addr).decode('utf-8')