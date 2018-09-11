import unittest
import os
import logging
import multiprocessing as mp
import collections
from Centaurus import *

class JsonListener(object):
    def __init__(self):
        pass
    def parseNumber(self, ctx):
        try:
            return float(ctx.read().decode('utf-8'))
        except ValueError:
            return 0.0
    def parseString(self, ctx):
        s = ctx.read().decode('utf-8')
        if s.startswith('"') and s.endswith('"'):
            return s[1:-1]
        else:
            return ""
    def parseBoolean(self, ctx):
        s = ctx.read().decode('utf-8').lower()
        return s == 'true'
    def parseNone(self, ctx):
        return None
    def parseObject(self, ctx):
        return ctx.value(1)
    def parseList(self, ctx):
        return ctx.value(1)
    def prependlist(self, obj, lst):
        lst.append(obj)
        return lst
    def parseListContent(self, ctx):
        if ctx.count() == 0:
            return []
        else:
            return self.prependlist(ctx.value(1), ctx.value(2))
    def parseListItems(self, ctx):
        if ctx.count() == 0:
            return []
        else:
            return self.prependlist(ctx.value(1), ctx.value(2))
    def parseDictionary(self, ctx):
        obj = ctx.value(1)
        if 'type' in obj:
            if obj['type'] == "Feature":
                if 'properties' in obj:
                    properties = obj['properties']
                    if 'STREET' in properties:
                        if properties['STREET'] != "JEFFERSON":
                            return {}
        return obj
    def parseDictionaryEntry(self, ctx):
        return (ctx.value(1), ctx.value(2))
    def parseDictionaryEntries(self, ctx):
        if ctx.count() == 0:
            return {}
        else:
            p = ctx.value(1)
            d = ctx.value(2)
            d[p[0]] = p[1]
            return d
    def parseDictionaryContent(self, ctx):
        if ctx.count() == 0:
            return {}
        else:
            p = ctx.value(1)
            d = ctx.value(2)
            d[p[0]] = p[1]
            return d

class Test_unittest1(unittest.TestCase):
    def test_grammar(self):
        grammar = Grammar(r"../grammar/json.cgr")
        grammar.print(r"../grammar/json.dot")
    def test_dry_parser(self):
        grammar = Grammar(r"../grammar/json.cgr")
        parser = Parser(grammar, True)
        #input_path = r"C:\Users\ihara\Downloads\sf-city-lots-json-master\sf-city-lots-json-master\citylots.json"
        input_path = r"/home/ihara/Downloads/sf-city-lots-json-master/citylots.json"
        st1_runner = Stage1Runner(input_path, parser, 8 * 1024 * 1024, 8)
        st1_runner.start()
        print("Runner started.", flush=True)
        st1_runner.wait()
    def test_wet_parser(self):
        grammar = Grammar(r"../grammar/json.cgr")
        parser = Parser(grammar)
        chaser = Chaser(grammar)
        #input_path = r"C:\Users\ihara\Downloads\sf-city-lots-json-master\sf-city-lots-json-master\citylots.json"
        input_path = r"/home/ihara/Downloads/sf-city-lots-json-master/citylots.json"
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
        context = Context(r"../grammar/json.cgr", 34)
        input_path = r"/home/ihara/Downloads/sf-city-lots-json-master/citylots.json"
        listener = JsonListener()
        context.attach(listener)
        context.start()
        context.parse(input_path)
        context.stop()

if __name__ == '__main__':
    mp.set_start_method('spawn')
    unittest.main()
