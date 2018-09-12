import logging
import sys
import collections
import traceback
from io import StringIO
from .CoreLib import CoreLib
from .Grammar import Grammar
from .Runner import *
from .Semantics import SemanticStore

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

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
            lhs_value = self.handlers[symbols[0].id - 1](ss)
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
            logger.debug("Writing page %d to bank %d..." % (self.page_index, index))
            self.channels[index].put(list(self.values))
            self.values = None
            logger.debug("Wrote page %d to bank %d" % (self.page_index, index))
        else:
            self.page_index = new_index
            self.values = collections.deque()
            logger.debug("Started parsing page %d" % (self.page_index,))

class Stage3ListenerAdapter(BaseListenerAdapter):
    def __init__(self, grammar, handler, channels, runner):
        super(Stage3ListenerAdapter, self).__init__(grammar, handler, channels, runner)
        self.page_values = []
        self.values = collections.deque()
        runner.attach(self.reduction_callback, self.transfer_callback)

    def reduction_callback(self, symbols, num):
        try:
            ss = SemanticStore(self.grammar, self.window, self.page_values, self.values, symbols, num)
            lhs_value = self.handlers[symbols[0].id - 1](ss)
            self.values.append(lhs_value)
            return len(self.values) - 1
        except:
            logger.debug('Exception in Stage3Listener:')
            logger.debug(traceback.format_exc())
            sys.exit()
        
    def transfer_callback(self, index, new_index):
        logger.debug("Accepting page %d from bank %d..." % (new_index, index))
        values = self.channels[index].get()
        #logging.debug(values)
        self.page_values.append(values)
        logger.debug("Accepted page %d from bank %d" % (new_index, index))
        logger.debug("Stack size = %d" % len(self.values))
