#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from .CoreLib import CoreLib
import ctypes

class Parser(object):
    CoreLib.ParserCreate.restype = ctypes.c_void_p
    CoreLib.ParserCreate.argtypes = [ctypes.c_void_p, ctypes.c_bool]
    CoreLib.ParserDestroy.argtypes = [ctypes.c_void_p]

    def __init__(self, grammar, dry = False):
        self.handle = CoreLib.ParserCreate(grammar.handle, dry)

    def __del__(self):
        CoreLib.ParserDestroy(self.handle)

class Chaser(object):
    CoreLib.ChaserCreate.restype = ctypes.c_void_p
    CoreLib.ChaserCreate.argtypes = [ctypes.c_void_p]
    CoreLib.ChaserDestroy.argtypes = [ctypes.c_void_p]

    def __init__(self, grammar):
        self.handle = CoreLib.ChaserCreate(grammar.handle)

    def __del__(self):
        CoreLib.ChaserDestroy(self.handle)