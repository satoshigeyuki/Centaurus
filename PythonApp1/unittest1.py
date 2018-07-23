import unittest
import os
import multiprocessing as mp
from Centaurus import *

class Test_unittest1(unittest.TestCase):
    def test_grammar(self):
        grammar = Grammar(r"..\grammar\json.cgr")
        grammar.print(r"..\grammar\json.dot")
    def test_dry_parser(self):
        grammar = Grammar(r"..\grammar\json.cgr")
        parser = Parser(grammar, True)
        input_path = r"C:\Users\ihara\Downloads\sf-city-lots-json-master\sf-city-lots-json-master\citylots.json"
        st1_runner = Stage1Runner(input_path, parser, 8 * 1024 * 1024, 8)
        st1_runner.start()
        st1_runner.wait()
    def test_wet_parser(self):
        grammar = Grammar(r"..\grammar\json.cgr")
        parser = Parser(grammar)
        chaser = Chaser(grammar)
        input_path = r"C:\Users\ihara\Downloads\sf-city-lots-json-master\sf-city-lots-json-master\citylots.json"
        pid = os.getpid()
        runners = []
        runners.append(Stage1Runner(input_path, parser, 8 * 1024 * 1024, 8))
        for i in range(4):
            runners.append(Stage2Runner(input_path, chaser, 8 * 1024 * 1024, 8, pid))
        runners.append(Stage3Runner(input_path, chaser, 8 * 1024 * 1024, 8, pid))
        for runner in runners:
            runner.start()
        for runner in runners:
            runner.wait()
    def test_wet_parser_mp(self):
        grammar = Grammar(r"..\grammar\json.cgr")
        parser = Parser(grammar)
        chaser = Chaser(grammar)
        input_path = r"C:\Users\ihara\Downloads\sf-city-lots-json-master\sf-city-lots-json-master\citylots.json"
        pid = os.getpid()
        runners = []
        runners.append(Stage1Runner(input_path, parser, 8 * 1024 * 1024, 8))
        for i in range(4):
            runners.append(Stage2Runner(input_path, chaser, 8 * 1024 * 1024, 8, pid))
        runners.append(Stage3Runner(input_path, chaser, 8 * 1024 * 1024, 8, pid))
        for runner in runners:
            runner.start()
        for runner in runners:
            runner.wait()

if __name__ == '__main__':
    unittest.main()
