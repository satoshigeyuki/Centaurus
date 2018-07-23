import unittest
import CoreLib
import Grammar

class Test_unittest1(unittest.TestCase):
    def test_corelib_load(self):
        corelib = CoreLib.get_instance()
    def test_grammar(self):
        grammar = Grammar.Grammar(r"..\grammar\json.cgr")
        grammar.print(r"..\grammar\json.dot")

if __name__ == '__main__':
    unittest.main()
