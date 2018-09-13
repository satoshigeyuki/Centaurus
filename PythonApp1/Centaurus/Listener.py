import logging
import sys
import collections
import traceback
import time
import ctypes
from io import StringIO
from .CoreLib import CoreLib
from .Grammar import Grammar
from .Runner import *

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

class SymbolEntry(ctypes.Structure):
    _fields_ = [('id', ctypes.c_int),
                ('key', ctypes.c_int),
                ('start', ctypes.c_long),
                ('end', ctypes.c_long)]

class BaseListenerAdapter(object):
    def __init__(self, grammar, channels, runner):
        self.grammar = grammar
        self.window = runner.get_window()
        self.channels = channels
    def read(self):
        start_addr = self.window + self.symbols[0].start
        end_addr = self.window + self.symbols[0].end
        return ctypes.string_at(ctypes.c_void_p(start_addr), end_addr - start_addr)
        #return ctypes.string_at(ctypes.c_void_p(start_addr), end_addr - start_addr).decode('utf-8')
    def count(self):
        return self.num - 1

"""
class Stage2BytecodeListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, runner):
        super(Stage2BytecodeListenerAdapter, self).__init__(grammar, channels, runner)
        self.values = None
        self.page_index = -1
        runner.attach(self.reduction_callback, self.transfer_callback)
        self.run_time = 0.0
        self.handlers = [None] * grammar.get_machine_num()
        if handler:
            for index in range(1, len(self.handlers) + 1):
                handler_name = grammar.get_machine_name(index)
                if handler_name in handler:
                    self.handlers[index - 1] = compile(handler[handler_name], '<string>', 'exec')

    def reduction_callback(self, symbols, num):
        try:
            self.symbols = symbols
            self.num = num
            ldict = {'ctx': self}
            exec(self.handlers[symbols[0].id - 1], globals(), ldict)
            for i in range(num - 1):
                self.values.pop()
            self.values.append(ldict['lhs'])
            return ((self.page_index + 1) << 20) | (len(self.values) - 1)
        except:
            logger.debug('Exception in Stage2Listener:')
            logger.debug(traceback.format_exc())
            sys.exit()
    
    def transfer_callback(self, index, new_index):
        if self.values:
            end_time = time.time()
            self.run_time += end_time - self.start_time
            logger.debug("Stage2 cumulative runtime = %f[s]" % (self.run_time,))
            self.channels[index].put(list(self.values))
            self.values = None
        else:
            self.page_index = new_index
            self.values = collections.deque()
            self.start_time = time.time()

    def value(self, index):
        return self.values[-self.num + index]
"""

class Stage2ListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, runner):
        super(Stage2ListenerAdapter, self).__init__(grammar, channels, runner)
        self.values = None
        self.page_index = -1
        runner.attach(self.reduction_callback, self.transfer_callback)
        self.run_time = 0.0
        self.handlers = [None] * grammar.get_machine_num()
        if handler:
            for index in range(1, len(self.handlers) + 1):
                handler_name = 'parse' + grammar.get_machine_name(index)
                if handler_name in dir(handler):
                    self.handlers[index - 1] = getattr(handler, handler_name)

    def reduction_callback(self, symbols, num):
        try:
            self.symbols = symbols
            self.num = num
            lhs_value = self.handlers[symbols[0].id - 1](self)
            for i in range(num - 1):
                self.values.pop()
            self.values.append(lhs_value)
            return ((self.page_index + 1) << 20) | (len(self.values) - 1)
        except:
            logger.debug('Exception in Stage2Listener:')
            logger.debug(traceback.format_exc())
            sys.exit()
    
    def transfer_callback(self, index, new_index):
        if self.values:
            end_time = time.time()
            self.run_time += end_time - self.start_time
            logger.debug("Stage2 cumulative runtime = %f[s]" % (self.run_time,))
            self.channels[index].put(list(self.values))
            self.values = None
        else:
            self.page_index = new_index
            self.values = collections.deque()
            self.start_time = time.time()

    def value(self, index):
        return self.values[-self.num + index]

"""
class Stage3BytecodeListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, runner):
        super(Stage3BytecodeListenerAdapter, self).__init__(grammar, channels, runner)
        self.page_values = []
        self.values = collections.deque()
        runner.attach(self.reduction_callback, self.transfer_callback)
        self.logger = logging.getLogger("Centaurus.Stage3Listener")
        self.logger.setLevel(logging.DEBUG)
        self.handlers = [None] * grammar.get_machine_num()
        if handler:
            for index in range(1, len(self.handlers) + 1):
                handler_name = grammar.get_machine_name(index)
                if handler_name in handler:
                    self.handlers[index - 1] = compile(handler[handler_name], '<string>', 'exec')

    def reduction_callback(self, symbols, num):
        try:
            self.symbols = symbols
            self.num = num
            ldict = {'ctx': self}
            exec(self.handlers[symbols[0].id - 1], globals(), ldict)
            self.values.append(ldict['lhs'])
            return len(self.values) - 1
        except:
            logger.debug('Exception in Stage3Listener:')
            logger.debug(traceback.format_exc())
            sys.exit()
        
    def transfer_callback(self, index, new_index):
        values = self.channels[index].get()
        self.page_values.append(values)

    def value(self, index):
        key = self.symbols[index].key & ((1 << 20) - 1)
        page = self.symbols[index].key >> 20
        if page == 0:
            return self.values[key]
        else:
            return self.page_values[page - 1][key]
"""

class Stage3ListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, runner):
        super(Stage3ListenerAdapter, self).__init__(grammar, channels, runner)
        self.page_values = []
        self.values = collections.deque()
        runner.attach(self.reduction_callback, self.transfer_callback)
        self.logger = logging.getLogger("Centaurus.Stage3Listener")
        self.logger.setLevel(logging.DEBUG)
        self.handlers = [None] * grammar.get_machine_num()
        if handler:
            for index in range(1, len(self.handlers) + 1):
                handler_name = 'parse' + grammar.get_machine_name(index)
                if handler_name in dir(handler):
                    self.handlers[index - 1] = getattr(handler, handler_name)

    def reduction_callback(self, symbols, num):
        try:
            self.symbols = symbols
            self.num = num
            """
            value_indices = [len(self.values)]
            for index in range(1, num):
                page_index = symbols[index].key >> 20
                value_index = symbols[index].key & ((1 << 20) - 1)
                if page_index == 0:
                    value_indices.append(value_index)
            self.logger.debug(value_indices)
            """
            lhs_value = self.handlers[symbols[0].id - 1](self)
            self.values.append(lhs_value)
            return len(self.values) - 1
        except:
            logger.debug('Exception in Stage3Listener:')
            logger.debug(traceback.format_exc())
            sys.exit()
        
    def transfer_callback(self, index, new_index):
        values = self.channels[index].get()
        self.page_values.append(values)

    def value(self, index):
        key = self.symbols[index].key & ((1 << 20) - 1)
        page = self.symbols[index].key >> 20
        if page == 0:
            return self.values[key]
        else:
            return self.page_values[page - 1][key]
