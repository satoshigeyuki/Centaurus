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

class XMLElement(object):
    def __init__(self, ctx):
        self.name = ctx.value(1)
        self.attr = {}
        for i in range(2, ctx.count()):
            attr = ctx.value(i)
            self.attr[attr[0]] = attr[1]
        self.body = ctx.value(ctx.count())
    def get_content(self):
        if len(self.body) == 0:
            return ""
        return self.body[0]
    def get_child_content(self, name):
        for item in self.body[1::2]:
            if item.name == name:
                return item.get_content()
        return ""

class XMLListener(object):
    def parseName(self, ctx):
        return ctx.read()
    def parseValue(self, ctx):
        return ctx.read().strip()[1:-1]
    def parseAttribute(self, ctx):
        return (ctx.value(1), ctx.value(2))
    def parseContent(self, ctx):
        return ctx.read()
    def parseElementBody(self, ctx):
        if ctx.count() == 0:
            return []
        else:
            ret = []
            for i in range(1, ctx.count()):
                ret.append(ctx.value(i))
            return ret
    def parseElement(self, ctx):
        e = XMLElement(ctx)
        if e.name == "article":
            if e.get_child_content("year").strip() == "1990":
                #logger.info("1990 article")
                return e
            else:
                return None
        elif e.name == "inproceedings" or e.name == "proceedings" or e.name == "book" or e.name == "incollection" or e.name == "phdthesis" or e.name == "masterthesis" or e.name == "www":
            return None
        return e
    def parseDocument(self, ctx):
        return ctx.value(2)
    def parseProlog(self, ctx):
        return None
    def parseDocType(self, ctx):
        return None

if __name__ == "__main__":
    logging.basicConfig(filename="app.log", filemode='w', level=logging.DEBUG)

    log_sink = LoggerManifold("")
    log_sink.start()

    #worker_num = [2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46]
    #worker_num = [34]
    #worker_num = [2, 6, 10, 14, 18, 22, 26, 30, 34]
    worker_num = [1, 2, 4, 6, 8, 10, 12, 14, 16]

    perf_log = open('perf.log', 'w')

    for n in worker_num:
        context = Context(r"../grammar/xml.cgr", n)
        input_path = r"/home/ihara/dblp.xml"
        listener = XMLListener()
        context.attach(listener)
        context.start()
        start_time = time.time()
        result = context.parse(input_path)
        end_time = time.time()
        context.stop()
        print("%d %f" % (n, end_time - start_time), file=perf_log)

        """
        result_log = open('result.log', 'w')
        features_list = result['features']
        for item in features_list:
            if isinstance(item, dict) and len(item) > 0:
                print(item, file=result_log)
        """

    log_sink.stop()
