#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import CoreLib
import ctypes

corelib = CoreLib.get_instance()

class Grammar(object):
    """EBNF grammar definition."""

    def __init__(self):
        self.handle = ctypes.c_void_p(corelib.GrammarCreate())
    def __enter__(self):
        return self
    def __exit__(self, exc_type, exc_value, traceback):
        corelib.GrammarDestroy(self.handle)