#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import CoreLib
import ctypes

corelib = CoreLib.get_instance()

class Stage1Runner(object):
    corelib.Stage1RunnerCreate.restype = ctypes.c_void_p
    corelib.Stage1RunnerCreate.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int]
    corelib.Stage1RunnerDestroy.argtypes = [ctypes.c_void_p]

    def __init__(self, filename, parser, bank_size, bank_num):
        self.handle = corelib.Stage1RunnerCreate(filename.encode('utf-8'), parser.handle, bank_size, bank_num)

    def __del__(self):
        corelib.Stage1RunnerDestroy(self.handle)

class Stage2Runner(object):
    corelib.Stage2RunnerCreate.restype = ctypes.c_void_p
    corelib.Stage2RunnerCreate.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int, ctypes.c_int]
    corelib.Stage2RunnerDestroy.argtypes = [ctypes.c_void_p]

    def __init__(self, filename, chaser, bank_size, bank_num, master_pid):
        self.handle = corelib.Stage2RunnerCreate(filename.encode('utf-8'), chaser.handle, bank_size, bank_num, master_pid)

    def __del__(self):
        corelib.Stage2RunnerDestroy(self.handle)

class Stage3Runner(object):
    corelib.Stage3RunnerCreate.restype = ctypes.c_void_p
    corelib.Stage3RunnerCreate.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int, ctypes.c_int]
    corelib.Stage3RunnerDestroy.argtypes = [ctypes.c_void_p]

    def __init__(self, filename, chaser, bank_size, bank_num, master_pid):
        self.handle = corelib.Stage3RunnerCreate(filename.encode('utf-8'), chaser.handle, bank_size, bank_num, master_pid)

    def __del__(self):
        corelib.Stage3RunnerDestroy(self.handle)

if __name__ == "__main__":
    pass