#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

from __future__ import print_function
import time
import os

os.environ['CENTAURUS_DL_PATH'] = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build")
from Centaurus import *

if __name__ == "__main__":
    #grammar = Grammar(r"../grammar/json2.cgr")
    grammar = Grammar(r"../grammar/xml.cgr")
    grammar.optimize()
    parser = Parser(grammar, True)
    #input_path = r"../datasets/citylots.json"
    input_path = r"../datasets/dblp.xml"
    st1_runner = Stage1Runner(input_path, parser, 8 * 1024 * 1024, 8);
    start_time = time.time()
    st1_runner.start()
    print("Runner started.", flush=True)
    st1_runner.wait()
    print("Runner finished.", flush=True)
    end_time = time.time()
    print("Elapsed time = %lf" % ((end_time - start_time) * 1E+3,))
