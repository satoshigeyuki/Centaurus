#!/usr/bin/env python3

import sys
import os
import logging
import time
import json

os.environ['CENTAURUS_DL_PATH'] = '../../build'
sys.path.append('../../src/python/')

from centaurus import *

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

class JsonListener(object):
    def parseRootObject(self, ctx):
        return ctx[0]
    def parseFeatureList(self, ctx):
        return list(ctx)
    def parseFeatureDict(self, ctx):
        if ctx:
            return json.loads(ctx.readbytes())
    def parsePropertyDict(self, ctx):
        if ctx:
            return True
    def parseTargetPropertyEntry(self, ctx):
        return True
    def defaultact(self, ctx):
        pass

if __name__ == '__main__':
    app_name = os.path.basename(sys.argv[0]).split('.')[0]
    num_workers = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    logging.basicConfig(filename='debug_{}-p{}.log'.format(app_name, num_workers), filemode='w', level=logging.DEBUG)

    start_time = time.time()
    context = Context('../../grammars/citylots.cgr')
    end_time = time.time()
    gen_time = end_time - start_time
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
            for item in result:
                if isinstance(item, dict) and len(item) > 0:
                    print(item, file=dst)
