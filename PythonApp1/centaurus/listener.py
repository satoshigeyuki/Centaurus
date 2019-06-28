import logging
import sys
import collections
import traceback
import time
import ctypes
from io import StringIO

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

class GrammarException(Exception):
    pass

class BaseListenerAdapter(object):
    default_handler_name = 'defaultact'

    def __init__(self, grammar, handler, channels, window):
        self.grammar = grammar
        self.window = window
        self.channels = channels
        self.handlers = [None] #NOTE: padding for 1-based symbol.id
        for index in range(1, grammar.get_machine_num() + 1):
            handler_name = 'parse' + grammar.get_machine_name(index)
            if hasattr(handler, handler_name):
                self.handlers.append(getattr(handler, handler_name))
            elif hasattr(handler, self.default_handler_name):
                self.handlers.append(getattr(handler, self.default_handler_name))
            else:
                raise GrammarException('Method %s for Nonterminal %s is missing.' % (handler_name, grammar.get_machine_name(index)))

    def invoke_hander(self, symbol, argc):
        self.symbol = symbol
        self.argc = argc
        lhs_value = self.handlers[symbol.id](self)
        del self.values[len(self.values) - argc:]
        if lhs_value is None:
            return 0
        else:
            self.values.append(lhs_value)
            return 1

    def readbytes(self):
        start_addr = self.window + self.symbol.start
        end_addr = self.window + self.symbol.end
        return ctypes.string_at(ctypes.c_void_p(start_addr), end_addr - start_addr)
    def read(self):
        return self.readbytes().decode()
    def __len__(self):
        return self.argc
    def __getitem__(self, index):
        return self.values[len(self.values) - self.argc + index]
    def __iter__(self):
        return (self.values[i] for i in range(len(self.values) - self.argc, len(self.values)))

class Stage2ListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, window):
        super(Stage2ListenerAdapter, self).__init__(grammar, handler, channels, window)
        self.values = None
        self.page_index = -1
        self.run_time = 0.0

    def reduction_callback(self, symbol, values, num_values):
        try:
            return self.invoke_hander(symbol[0], num_values)
        except:
            logger.debug('Exception in Stage2Listener:')
            logger.debug(traceback.format_exc())
            sys.exit()

    def transfer_callback(self, index, new_index):
        if self.values is not None:
            end_time = time.time()
            self.run_time += end_time - self.start_time
            logger.debug("Stage2 cumulative runtime = %f[s]" % (self.run_time,))
            self.channels[index].put(self.values)
            self.values = None
        else:
            self.page_index = new_index
            self.values = []
            self.start_time = time.time()

class Stage3ListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, window):
        super(Stage3ListenerAdapter, self).__init__(grammar, handler, channels, window)
        self.valueiters = collections.deque()
        self.values = None
        self.start_time = time.time()
        self.run_time = 0.0

    def reduction_callback(self, symbol, values, num_values):
        try:
            shift_count = values[0]
            while shift_count > 0:
                for v in self.valueiters[0]:
                    self.values.append(v)
                    shift_count -= 1
                    if shift_count == 0:
                        break
                else:
                    self.valueiters.popleft()
            return self.invoke_hander(symbol[0], num_values)
        except:
            logger.debug('Exception in Stage3Listener:')
            logger.debug(traceback.format_exc())
            sys.exit()

    def transfer_callback(self, index, new_index):
        values = self.channels[index].get()
        if self.values is None:
            self.values = values
        else:
            self.valueiters.append(iter(values))
        end_time = time.time()
        self.run_time += end_time - self.start_time
        self.start_time = end_time
        logger.debug("Stage3 cumulative runtime = %f[s]" % (self.run_time,))
        logger.debug("Accepted %d values" % len(values))
