#!/usr/bin/env python
# -*- coding: UTF-8 -*-

from __future__ import print_function
import os
import logging
import multiprocessing as mp
import time

os.environ['CENTAURUS_DL_PATH'] = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build")

from Centaurus import *

logger = getLogger(__name__)
logger.setLevel(logging.DEBUG)

class JsonListener(object):
    def parseNumber(self, ctx):
        try:
            return float(ctx.read())
        except ValueError:
            return 0.0
    def parseString(self, ctx):
        #return ctx.read().decode('utf-8')[1:-1]
        return ctx.read()[1:-1]
    def parseBoolean(self, ctx):
        #return ctx.read().decode('utf-8').lower() == 'true'
        return ctx.read().lower() == 'true'
    def parseNone(self, ctx):
        return None
    def parseObject(self, ctx):
        return ctx.value(1)
    def parseList(self, ctx):
        ret = []
        for i in range(1, ctx.count() + 1):
            ret.append(ctx.value(i))
        return ret
    def parseDictionary(self, ctx):
        ret = {}
        for i in range(1, ctx.count() + 1):
            p = ctx.value(i)
            ret[p[0]] = p[1]
        if 'type' in ret:
            if ret['type'] == "Feature":
                if 'properties' in ret:
                    properties = ret['properties']
                    if 'STREET' in properties:
                        if properties['STREET'] != "JEFFERSON":
                            return {}
        return ret
        """obj = ctx.value(1)
        return obj"""
    def parseDictionaryEntry(self, ctx):
        return (ctx.value(1), ctx.value(2))

if __name__ == "__main__":
    logging.basicConfig(filename="app.log", filemode='w', level=logging.DEBUG)

    log_sink = LoggerManifold("")
    log_sink.start()

    #worker_num = [2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46]
    worker_num = [34]

    perf_log = open('perf.log', 'w')

    for n in worker_num:
        context = Context(r"../grammar/json2.cgr", n)
        input_path = r"/home/ihara/Downloads/sf-city-lots-json-master/citylots.json"
        listener = JsonListener()
        context.attach(listener)
        context.start()
        start_time = time.time()
        result = context.parse(input_path)
        end_time = time.time()
        context.stop()
        print("%d %f" % (n, end_time - start_time), file=perf_log)

        result_log = open('result.log', 'w')
        features_list = result['features']
        for item in features_list:
            if isinstance(item, dict) and len(item) > 0:
                print(item, file=result_log)

    log_sink.stop()
