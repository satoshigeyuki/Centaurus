import logging
import sys
import collections
import traceback
from io import StringIO
from .CoreLib import CoreLib
from .Grammar import Grammar
from .Runner import *
from .Semantics import SemanticStore

def default_callback(symbols, num):
    return 0

class BaseListenerAdapter(object):
    def __init__(self, grammar, handler, channels, runner):
        self.grammar = grammar
        self.window = runner.get_window()
        self.channels = channels
        self.handlers = [None] * grammar.get_machine_num()
        if handler:
            for index in range(1, len(self.handlers) + 1):
                handler_name = 'parse' + grammar.get_machine_name(index)
                if handler_name in dir(handler):
                    self.handlers[index - 1] = getattr(handler, handler_name)
        
class Stage2ListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, runner):
        super(Stage2ListenerAdapter, self).__init__(grammar, handler, channels, runner)
        self.values = None
        self.page_index = -1
        runner.attach(self.reduction_callback, self.transfer_callback)

    def reduction_callback(self, symbols, num):
        try:
            ss = SemanticStore(self.grammar, self.window, None, self.values, symbols, num)
            if self.handlers[symbols[0].id - 1]:
                lhs_value = self.handlers[symbols[0].id - 1](ss)
            else:
                lhs_value = None
            for i in range(num - 1):
                self.values.pop()
            self.values.append(lhs_value)
            return ((self.page_index + 1) << 20) | (len(self.values) - 1)
        except:
            logging.debug('Exception in Stage2Listener:')
            logging.debug(traceback.format_exc())
            sys.exit()
    
    def transfer_callback(self, index, new_index):
        if self.values:
            logging.debug("Writing page %d to bank %d..." % (self.page_index, index))
            self.channels[index].put(list(self.values))
            self.values = None
            logging.debug("Wrote page %d to bank %d" % (self.page_index, index))
        else:
            self.page_index = new_index
            self.values = collections.deque()
            logging.debug("Started parsing page %d" % (self.page_index,))

class Stage3ListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, runner):
        super(Stage3ListenerAdapter, self).__init__(grammar, handler, channels, runner)
        self.page_values = []
        self.values = collections.deque()
        runner.attach(self.reduction_callback, self.transfer_callback)

    def reduction_callback(self, symbols, num):
        try:
            ss = SemanticStore(self.grammar, self.window, self.page_values, self.values, symbols, num)
            if self.handlers[symbols[0].id - 1]:
                lhs_value = self.handlers[symbols[0].id - 1](ss)
            else:
                lhs_value = None
            self.values.append(lhs_value)
            return len(self.values) - 1
        except:
            logging.debug('Exception in Stage3Listener:')
            logging.debug(traceback.format_exc())
            sys.exit()
        
    def transfer_callback(self, index, new_index):
        logging.debug("Accepting page %d from bank %d..." % (new_index, index))
        values = self.channels[index].get()
        self.page_values.append(values)
        logging.debug("Accepted page %d from bank %d" % (new_index, index))
        logging.debug("Stack size = %d" % len(self.values))
