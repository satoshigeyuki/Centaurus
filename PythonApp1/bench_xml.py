#!/usr/bin/env python

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
        it = iter(ctx)
        self.name = next(it)
        self.attr = dict(next(it) for _ in range(len(ctx) - 2))
        self.body = next(it)
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
        return tuple(ctx)
    def parseContent(self, ctx):
        return ctx.read()
    def parseElementBody(self, ctx):
        return list(ctx)
    def parseElement(self, ctx):
        e = XMLElement(ctx)
        if e.name == "article":
            if e.get_child_content("year").strip() == "1990":
                #logger.info("1990 article")
                return e
            else:
                return None
        elif e.name == "inproceedings" or e.name == "proceedings" or e.name == "book" or e.name == "incollection" or e.name == "phdthesis" or e.name == "mastersthesis" or e.name == "www":
            return None
        return e
    def parseDocument(self, ctx):
        return ctx[0]
    def parseProlog(self, ctx):
        return None
    def parseDocType(self, ctx):
        return None
    def parseComment(self, ctx):
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
        input_path = r"../datasets/dblp.xml"
        listener = XMLListener()
        context.attach(listener)
        context.start()
        start_time = time.time()
        result = context.parse(input_path)
        end_time = time.time()
        context.stop()
        print("%d %f" % (n, end_time - start_time), file=perf_log)

        result_log = open('result.log', 'w')
        def print_xml(doc):
            if isinstance(doc, XMLElement):
                print('<' + doc.name, end='',  file=result_log)
                n = len(doc.body)
                if n == 0:
                    print('>', end='', file=result_log)
                elif n > 1:
                    print('>', end='',  file=result_log)
                    if doc.name in ('dblp', 'article'):
                        print(file=result_log)
                    for i in range(n - 1):
                        print_xml(doc.body[i])
                    print('</' + doc.body[n - 1] + '>', file=result_log)
                else:
                    assert False
            elif isinstance(doc, str):
                print(doc.replace('\n', ''), end='', file=result_log)
        print_xml(result)

    log_sink.stop()
