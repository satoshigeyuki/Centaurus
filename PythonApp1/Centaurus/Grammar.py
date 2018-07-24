#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from .CoreLib import CoreLib
import ctypes

class GrammarAction(object):
    def __init__(self):
        pass

EnumMachinesCallback = ctypes.CFUNCTYPE(None, ctypes.c_wchar_p, ctypes.c_int)

class Grammar(object):
    """EBNF grammar definition."""
    CoreLib.GrammarCreate.restype = ctypes.c_void_p
    CoreLib.GrammarCreate.argtypes = [ctypes.c_char_p]
    CoreLib.GrammarDestroy.argtypes = [ctypes.c_void_p]
    #corelib.GrammarAddRule.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_wchar_p]
    CoreLib.GrammarPrint.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
    CoreLib.GrammarEnumMachines.argtypes = [ctypes.c_void_p, EnumMachinesCallback]

    def __init__(self, filename):
        self.handle = CoreLib.GrammarCreate(filename.encode('utf-8'))
        self.actions = []
        self.ids = {}
        CoreLib.GrammarEnumMachines(self.handle, EnumMachinesCallback(self.enum_machines_callback))
    def enum_machines_callback(self, name, id):
        self.ids[name] = id
        #print("%s: %d" % (name, id))
    def __del__(self):
        CoreLib.GrammarDestroy(self.handle)
    def print(self, filename):
        CoreLib.GrammarPrint(self.handle, filename.encode('utf-8'), 4)