import logging
import ctypes
from .CoreLib import CoreLib

class SymbolEntry(ctypes.Structure):
    _fields_ = [('id', ctypes.c_int),
                ('key', ctypes.c_int),
                ('start', ctypes.c_long),
                ('end', ctypes.c_long)]

class SemanticStore(object):
    def __init__(self, grammar, window, page_values, values, symbols, num):
        self.grammar = grammar
        self.window = window
        self.page_values = page_values
        self.values = values
        self.symbols = symbols
        self.num = num
    def read(self):
        start_addr = self.window + self.symbols[0].start
        end_addr = self.window + self.symbols[0].end
        return ctypes.string_at(ctypes.c_void_p(start_addr), end_addr - start_addr)
        #return ctypes.string_at(ctypes.c_void_p(start_addr), end_addr - start_addr).decode('utf-8')
    def value(self, index):
        if index >= self.num:
            return None
        else:
            if self.page_values is None:
                return self.values[-self.num + index]
            else:
                key = self.symbols[index].key & ((1 << 20) - 1)
                page = self.symbols[index].key >> 20
                if page == 0:
                    return self.values[key]
                else:
                    return self.page_values[page - 1][key]
    def count(self):
        return self.num - 1
