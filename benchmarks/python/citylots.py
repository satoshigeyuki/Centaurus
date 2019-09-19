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

class JsonListener(object):
    def parseNumber(self, ctx):
        try:
            return float(ctx.read())
        except ValueError:
            return 0.0
    def parseString(self, ctx):
        return ctx.read().strip()[1:-1]
    def parseTrue(self, ctx):
        return True
    def parseFalse(self, ctx):
        return False
    def parseNull(self, ctx):
        pass
    def parseObject(self, ctx):
        if ctx:
            return ctx[0]
    def parseList(self, ctx):
        return list(ctx)
    def parseDictionary(self, ctx):
        ret = dict(ctx)
        if ret.get('type') != 'Feature' or ret.get('properties', {}).get('STREET') == 'JEFFERSON':
            return ret
    def parseDictionaryEntry(self, ctx):
        return tuple(ctx) if len(ctx) == 2 else (ctx[0], None)

if __name__ == '__main__':
    app_name = os.path.basename(sys.argv[0]).split('.')[0]
    num_workers = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    logging.basicConfig(filename='debug_{}-p{}.log'.format(app_name, num_workers), filemode='w', level=logging.DEBUG)

    context = Context('../../grammars/json2.cgr')
    input_path = '../../datasets/citylots.json'
    listener = JsonListener()
    context.attach(listener)
    context.start(num_workers)
    start_time = time.time()
    result = context.parse(input_path)
    end_time = time.time()
    context.stop()
    print(num_workers, end_time - start_time)

    if len(sys.argv) > 2 and sys.argv[2].lower() == 'debug':
        with open('result_{}.log'.format(app_name), 'w') as dst:
            features_list = result['features']
            for item in features_list:
                if isinstance(item, dict) and len(item) > 0:
                    print(item, file=dst)

