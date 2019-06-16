#!/usr/bin/env python3

import time
# import xml.parsers.expat
from xml.dom.minidom import parse

input_path = '../datasets/dblp.xml'
start_time = time.time()
# parser = xml.parsers.expat.ParserCreate()
# result = parser.Parse(open(input_path, 'rb').read())
dom = parse(input_path)
end_time = time.time()
print(end_time - start_time)
