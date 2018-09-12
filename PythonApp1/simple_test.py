#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import os
import logging
import multiprocessing as mp

os.environ['CENTAURUS_DL_PATH'] = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build")

from Centaurus import *

logger = getLogger(__name__)
logger.setLevel(logging.DEBUG)

class JsonListener(object):
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

if __name__ == "__main__":
    mp.set_start_method('spawn')

    logging.basicConfig(filename="app.log", level=logging.DEBUG)

    log_sink = LoggerManifold("")
    log_sink.start()

    context = Context(r"../grammar/json.cgr", 34)
    input_path = r"/home/ihara/Downloads/sf-city-lots-json-master/citylots.json"
    listener = JsonListener()
    context.attach(listener)
    context.start()
    result = context.parse(input_path)
    result_log = open('result.log', 'w')
    features_list = result['features']
    for item in features_list:
        if isinstance(item, dict) and len(item) > 0:
            print(item, file=result_log)
    context.stop()

    log_sink.stop()
