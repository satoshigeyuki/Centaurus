#!/bin/sh

trials=15

for app in pugixml_load_mmap pugixml_load_file
do
    rm -fv perf_${app}.log
    for _ in $(seq $trials)
    do
        ./build/${app} ../../datasets/dblp.xml >> perf_${app}.log
    done
done

for app in rapidjson_load_mmap rapidjson_load_file rapidjson_read
do
    rm -fv perf_${app}.log
    for _ in $(seq $trials)
    do
        ./build/${app} ../../datasets/citylots.json >> perf_${app}.log
    done
done
