#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import print_function
from .CoreLib import CoreLib
import ctypes
from logging import getLogger
from .Exception import GrammarException

logger = getLogger(__name__)

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
        self.names = [''] * len(self.ids)
        for name, id in self.ids.items():
            self.names[id - 1] = name
    def enum_machines_callback(self, name, id):
        self.ids[name] = id
    def __del__(self):
        CoreLib.GrammarDestroy(self.handle)
    def print(self, filename):
        CoreLib.GrammarPrint(self.handle, filename.encode('utf-8'), 4)
    def get_machine_id(self, name):
        return self.ids[name]
    def get_machine_name(self, id):
        return self.names[id - 1]
    def get_machine_num(self):
        return len(self.ids)
    def validate_handler(self, handler):
        for index in range(1, len(self.ids) + 1):
            handler_name = 'parse' + self.names[index - 1]
            if not handler_name in dir(handler):
                raise GrammarException("Handler for Nonterminal %s is missing." % self.names[index - 1])
