#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import CoreLib
import ctypes

corelib = CoreLib.get_instance()

class Grammar(object):
    """EBNF grammar definition."""
    corelib.GrammarCreate.restype = ctypes.c_void_p
    corelib.GrammarDestroy.argtypes = [ctypes.c_void_p]
    corelib.GrammarAddRule.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_wchar_p]
    corelib.GrammarPrint.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]

    def __init__(self):
        self.handle = corelib.GrammarCreate()
    def __del__(self):
        corelib.GrammarDestroy(self.handle)
    def add_rule(self, lhs, rhs):
        corelib.GrammarAddRule(self.handle, lhs, rhs)
    def print(self, filename):
        corelib.GrammarPrint(self.handle, filename.encode('utf-8'), 4)