from .CoreLib import CoreLib
from .Grammar import Grammar
from .Runner import *
from .Semantics import SemanticStore

class ListenerAdapter(object):
    def __init__(self, grammar, handler, runner):
        self.grammar = grammar
        self.handler = handler
        self.window = runner.get_window()
        runner.attach(self.callback)
    def callback(self, symbols, num):
        print("Callback invoked.")
        ss = SemanticStore(self.grammar, self.window, symbols, num)
        lhs = self.grammar.get_machine_name(symbols[0].id)
        if lhs == 'Number':
            pass
        if self.handler:
            if 'parse' + lhs in self.handler.__dict__:
                result = getattr(self.handler, 'parse' + lhs)(ss)
