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
                ('start', ctypes.c_long),
                ('end', ctypes.c_long)]

class BaseListenerAdapter(object):
    def __init__(self, grammar, channels, runner):
        self.grammar = grammar
        self.window = runner.get_window()
        self.channels = channels
    def read(self):
        start_addr = self.window + self.symbol.start
        end_addr = self.window + self.symbol.end
        return ctypes.string_at(ctypes.c_void_p(start_addr), end_addr - start_addr)
    def count(self):
        return self.argc

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

    def reduction_callback(self, symbol, values, num_values):
        try:
            self.symbol = symbol[0]
            self.argc = num_values
            self.argv = [self.values.pop() for _ in range(num_values)]
            self.argv.reverse()
            lhs_value = self.handlers[self.symbol.id - 1](self)
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
        return self.argv[index - 1]
    def all(self):
        return self.argv

class Stage3ListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, runner):
        super(Stage3ListenerAdapter, self).__init__(grammar, channels, runner)
        self.page_values = []
        self.values = collections.deque()
        runner.attach(self.reduction_callback, self.transfer_callback)
        self.start_time = time.time()
        self.run_time = 0.0
        self.logger = logging.getLogger("Centaurus.Stage3Listener")
        self.logger.setLevel(logging.DEBUG)
        self.handlers = [None] * grammar.get_machine_num()
        if handler:
            for index in range(1, len(self.handlers) + 1):
                handler_name = 'parse' + grammar.get_machine_name(index)
                if handler_name in dir(handler):
                    self.handlers[index - 1] = getattr(handler, handler_name)

    def reduction_callback(self, symbol, values, num_values):
        try:
            self.symbol = symbol[0]
            self.argv = values
            self.argc = num_values
            lhs_value = self.handlers[self.symbol.id - 1](self)
            self.values.append(lhs_value)
            return len(self.values) - 1
        except:
            logger.debug('Exception in Stage3Listener:')
            logger.debug(traceback.format_exc())
            sys.exit()
        
    def transfer_callback(self, index, new_index):
        values = self.channels[index].get()
        self.page_values.append(values)
        end_time = time.time()
        self.run_time += end_time - self.start_time
        self.start_time = end_time
        logger.debug("Stage3 cumulative runtime = %f[s]" % (self.run_time,))
        logger.debug("Accepted %d values" % len(values))

    def value(self, index):
        key = self.argv[index - 1] & ((1 << 20) - 1)
        page = self.argv[index - 1] >> 20
        if page == 0:
            return self.values[key]
        else:
            return self.page_values[page - 1][key]
    def all(self):
        return [self.value(i) for i in range(1, self.argc + 1)]
