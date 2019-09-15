#!/usr/bin/env python3

import time, json

input_path = '../datasets/citylots.json'
start_time = time.time()
result = json.load(open(input_path))
# jefferson_list = list(f for f in result['features'] if f.get('properties', {}).get('STREET') == 'JEFFERSON')
end_time = time.time()
print(end_time - start_time)
