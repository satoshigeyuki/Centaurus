import logging
import sys
import collections
import traceback
import time
import ctypes
from io import StringIO
from .CoreLib import CoreLib
from .Grammar import Grammar
from .Exception import GrammarException
from .Runner import *

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

class SymbolEntry(ctypes.Structure):
    _fields_ = [('id', ctypes.c_int),
                ('start', ctypes.c_long),
                ('end', ctypes.c_long)]

class BaseListenerAdapter(object):
    def __init__(self, grammar, handler, channels, runner):
        self.grammar = grammar
        self.window = runner.get_window()
        self.channels = channels
        self.handlers = [None] #NOTE: padding for 1-based symbol.id
        for index in range(1, grammar.get_machine_num() + 1):
            handler_name = 'parse' + grammar.get_machine_name(index)
            if hasattr(handler, handler_name):
                self.handlers.append(getattr(handler, handler_name))
            else:
                raise GrammarException('Method %s for Nonterminal %s is missing.' % (handler_name, grammar.get_machine_name(index)))

    def invoke_hander(self, symbol, argc):
        self.symbol = symbol
        self.argc = argc
        return self.handlers[symbol.id](self)

    def read(self):
        start_addr = self.window + self.symbol.start
        end_addr = self.window + self.symbol.end
        return ctypes.string_at(ctypes.c_void_p(start_addr), end_addr - start_addr)
    def __len__(self):
        return self.argc

class Stage2ListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, runner):
        super(Stage2ListenerAdapter, self).__init__(grammar, handler, channels, runner)
        self.values = None
        self.page_index = -1
        runner.attach(self.reduction_callback, self.transfer_callback)
        self.run_time = 0.0

    def reduction_callback(self, symbol, values, num_values):
        try:
            lhs_value = self.invoke_hander(symbol[0], num_values)
            del self.values[len(self.values) - num_values:]
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
            self.values = []
            self.start_time = time.time()

    def __getitem__(self, index):
        return self.values[len(self.values) - self.argc + index]
    def __iter__(self):
        return (self.values[i] for i in range(len(self.values) - self.argc, len(self.values)))


class Stage3ListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, runner):
        super(Stage3ListenerAdapter, self).__init__(grammar, handler, channels, runner)
        self.page_values = []
        self.values = []
        runner.attach(self.reduction_callback, self.transfer_callback)
        self.start_time = time.time()
        self.run_time = 0.0

    def reduction_callback(self, symbol, values, num_values):
        try:
            self.argv = values
            lhs_value = self.invoke_hander(symbol[0], num_values)
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

    def __getitem__(self, index):
        key = self.argv[index] & ((1 << 20) - 1)
        page = self.argv[index] >> 20
        if page == 0:
            return self.values[key]
        else:
            return self.page_values[page - 1][key]
    def __iter__(self):
        return (self[i] for i in range(len(self)))
