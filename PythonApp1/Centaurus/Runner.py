#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from .CoreLib import CoreLib
import ctypes

class SymbolEntry(ctypes.Structure):
    _fields_ = [('id', ctypes.c_int),
                ('key', ctypes.c_long),
                ('start', ctypes.c_long),
                ('end', ctypes.c_long)]

ReductionListener = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(SymbolEntry), ctypes.c_int)
TransferListener = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_int)

class BaseRunner(object):
    CoreLib.RunnerDestroy.argtypes = [ctypes.c_void_p]
    CoreLib.RunnerStart.argtypes = [ctypes.c_void_p]
    CoreLib.RunnerWait.argtypes = [ctypes.c_void_p]
    CoreLib.RunnerRegisterListener.argtypes = [ctypes.c_void_p, ReductionListener, TransferListener]
    CoreLib.RunnerGetWindow.restype = ctypes.c_void_p
    CoreLib.RunnerGetWindow.argtypes = [ctypes.c_void_p]

    def __init__(self, handle):
        self.handle = handle

    def __del__(self):
        CoreLib.RunnerDestroy(self.handle)

    def start(self):
        CoreLib.RunnerStart(self.handle)

    def wait(self):
        CoreLib.RunnerWait(self.handle)

    def attach(self, listener, xferlistener):
        self.listener = ReductionListener(listener)
        self.xferlistener = TransferListener(xferlistener)
        CoreLib.RunnerRegisterListener(self.handle, self.listener, self.xferlistener)

    def get_window(self):
        return CoreLib.RunnerGetWindow(self.handle)

class Stage1Runner(BaseRunner):
    CoreLib.Stage1RunnerCreate.restype = ctypes.c_void_p
    CoreLib.Stage1RunnerCreate.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int]

    def __init__(self, filename, parser, bank_size, bank_num):
        super(Stage1Runner, self).__init__(CoreLib.Stage1RunnerCreate(filename.encode('utf-8'), parser.handle, bank_size, bank_num))

class Stage2Runner(BaseRunner):
    CoreLib.Stage2RunnerCreate.restype = ctypes.c_void_p
    CoreLib.Stage2RunnerCreate.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_int, ctypes.c_int]

    def __init__(self, filename, bank_size, bank_num, master_pid):
        super(Stage2Runner, self).__init__(CoreLib.Stage2RunnerCreate(filename.encode('utf-8'), bank_size, bank_num, master_pid))

class Stage3Runner(BaseRunner):
    CoreLib.Stage3RunnerCreate.restype = ctypes.c_void_p
    CoreLib.Stage3RunnerCreate.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_int, ctypes.c_int]

    def __init__(self, filename, bank_size, bank_num, master_pid):
        super(Stage3Runner, self).__init__(CoreLib.Stage3RunnerCreate(filename.encode('utf-8'), bank_size, bank_num, master_pid))
