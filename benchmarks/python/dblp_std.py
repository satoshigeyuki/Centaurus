#!/usr/bin/env python3

import sys
import os
import logging
import multiprocessing as mp
import time

os.environ['CENTAURUS_DL_PATH'] = '../../build'
sys.path.append('../../src/python/')

from centaurus import *

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

class XMLElement(object):
    __slots__ = ('name', 'attr', 'body')
    def __init__(self, ctx):
        it = iter(ctx)
        self.name = next(it)
        self.attr = dict(next(it) for _ in range(len(ctx) - 2))
        self.body = next(it)
    def content(self):
        return self.body[0] if self.body else ''
    def children(self):
        return (item for item in self.body if isinstance(item, XMLElement))

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
        if e.name == 'article':
            for c in e.children():
                if c.name == 'year' and c.content() == '1990':
                    return e
        elif e.name not in {'inproceedings', 'proceedings', 'book', 'incollection', 'phdthesis', 'mastersthesis', 'www'}:
            return e
    def parseDocument(self, ctx):
        return ctx[0]
    def defaultact(self, ctx):
        pass

def print_xml(doc, dstfile):
    if isinstance(doc, XMLElement):
        print('<' + doc.name, end='',  file=dstfile)
        n = len(doc.body)
        if n == 0:
            print('>', end='', file=dstfile)
        elif n > 1:
            print('>', end='',  file=dstfile)
            if doc.name in ('dblp', 'article'):
                print(file=dstfile)
            for i in range(n - 1):
                print_xml(doc.body[i], dstfile)
            print('</' + doc.body[n - 1] + '>', file=dstfile)
        else:
            assert False
    elif isinstance(doc, str):
        print(doc.replace('\n', ''), end='', file=dstfile)

if __name__ == '__main__':
    app_name = os.path.basename(sys.argv[0]).split('.')[0]
    num_workers = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    logging.basicConfig(filename='debug_{}-p{}.log'.format(app_name, num_workers), filemode='w', level=logging.DEBUG)

    context = Context('../../gramamrs/xml.cgr')
    input_path = '../../datasets/dblp.xml'
    listener = XMLListener()
    context.attach(listener)
    context.start(num_workers)
    start_time = time.time()
    result = context.parse(input_path)
    end_time = time.time()
    context.stop()
    print(num_workers, end_time - start_time)

    if len(sys.argv) > 2 and sys.argv[2].lower() == 'debug':
        with open('result_{}.log'.format(app_name), 'w') as dst:
            print_xml(result, dst)
