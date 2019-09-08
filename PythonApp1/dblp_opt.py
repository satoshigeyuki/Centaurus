#!/usr/bin/env python3

import sys
import os
import logging
import multiprocessing as mp
import time

os.environ['CENTAURUS_DL_PATH'] = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'build')

from centaurus import *

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

class DBLPListener(object):
    def parseDocument(self, ctx):
        return ctx[0]
    def parseDblpRoot(self, ctx):
        return list(ctx)
    def parseArticle(self, ctx):
        if ctx:
            return ctx.read()
    def parseYearInfo(self, ctx):
        return True if ctx else None
    def parseTargetYearInfo(self, ctx):
        return True
    def defaultact(self, ctx):
        pass

if __name__ == '__main__':
    app_name = os.path.basename(sys.argv[0]).split('.')[0]
    num_workers = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    logging.basicConfig(filename='debug_{}-p{}.log'.format(app_name, num_workers), filemode='w', level=logging.DEBUG)

    context = Context('../grammar/dblp.cgr')
    input_path = '../datasets/dblp.xml'
    listener = DBLPListener()
    context.attach(listener)
    context.start(num_workers)
    start_time = time.time()
    result = context.parse(input_path)
    end_time = time.time()
    context.stop()
    print(num_workers, end_time - start_time)

    if len(sys.argv) > 2 and sys.argv[2].lower() == 'size':
        print(len(result))
    if len(sys.argv) > 2 and sys.argv[2].lower() == 'debug':
        with open('result_{}.log'.format(app_name), 'w') as dst:
            for s in result:
                print(s)
