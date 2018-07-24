import logging
from .CoreLib import CoreLib
from .Grammar import Grammar
from .Runner import *
from .Semantics import SemanticStore

def default_callback(symbols, num):
    return 0

class ListenerAdapter(object):
    def __init__(self, grammar, handler, runner):
        self.grammar = grammar
        self.handler = handler
        self.window = runner.get_window()
        runner.attach(self.callback)
    def callback(self, symbols, num):
        return 0
        """ss = SemanticStore(self.grammar, self.window, symbols, num)
        lhs = self.grammar.get_machine_name(symbols[0].id)
        if self.handler:
            if 'parse' + lhs in self.handler.__dict__:
                result = getattr(self.handler, 'parse' + lhs)(ss)"""
