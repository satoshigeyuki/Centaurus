#!/usr/bin/env python3

import os
import ctypes
import platform

import logging
logger = logging.getLogger(__name__)

def load_dll():
    dl_path_env = os.getenv("CENTAURUS_DL_PATH", "")
    if platform.uname()[0] == "Windows":
        dl_path = os.path.join(dl_path_env, "libpycentaurus.dll")
    elif platform.uname()[0] == "Linux":
        dl_path = os.path.join(dl_path_env, "libpycentaurus.so")
    return ctypes.CDLL(dl_path, mode=ctypes.RTLD_GLOBAL)

CoreLib = load_dll()

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
    CoreLib.GrammarOptimize.argtypes = [ctypes.c_void_p]

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
    def optimize(self):
        CoreLib.GrammarOptimize(self.handle)
    def get_machine_id(self, name):
        return self.ids[name]
    def get_machine_name(self, id):
        return self.names[id - 1]
    def get_machine_num(self):
        return len(self.ids)

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

class SymbolEntry(ctypes.Structure):
    _fields_ = [('id', ctypes.c_int),
                ('start', ctypes.c_long),
                ('end', ctypes.c_long)]

ReductionListener = ctypes.CFUNCTYPE(ctypes.c_long, ctypes.POINTER(SymbolEntry), ctypes.POINTER(ctypes.c_long), ctypes.c_int)
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
