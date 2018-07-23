#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import CoreLib
import ctypes

corelib = CoreLib.get_instance()

class GrammarAction(object):
    def __init__(self):
        pass

class Grammar(object):
    """EBNF grammar definition."""
    corelib.GrammarCreate.restype = ctypes.c_void_p
    corelib.GrammarCreate.argtypes = [ctypes.c_char_p]
    corelib.GrammarDestroy.argtypes = [ctypes.c_void_p]
    #corelib.GrammarAddRule.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_wchar_p]
    corelib.GrammarPrint.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]

    def __init__(self, filename):
        self.handle = corelib.GrammarCreate(filename.encode('utf-8'))
        self.actions = []
    def __del__(self):
        corelib.GrammarDestroy(self.handle)
    def print(self, filename):
        corelib.GrammarPrint(self.handle, filename.encode('utf-8'), 4)