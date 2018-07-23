#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import CoreLib
import ctypes

corelib = CoreLib.get_instance()

class Parser(object):
    corelib.ParserCreate.restype = ctypes.c_void_p
    corelib.ParserCreate.argtypes = [ctypes.c_void_p]
    corelib.ParserDestroy.argtypes = [ctypes.c_void_p]

    def __init__(self, grammar):
        self.handle = corelib.ParserCreate(grammar.handle)

    def __del__(self):
        corelib.ParserDestroy(self.handle)

class Chaser(object):
    corelib.ChaserCreate.restype = ctypes.c_void_p
    corelib.ChaserCreate.argtypes = [ctypes.c_void_p]
    corelib.ChaserDestroy.argtypes = [ctypes.c_void_p]

    def __init__(self, grammar):
        self.handle = corelib.ChaserCreate(grammar.handle)

    def __del__(self):
        corelib.ChaserDestroy(self.handle)