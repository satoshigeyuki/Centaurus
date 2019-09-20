#!/bin/sh

trials=15

app=dry_dblp
rm -fv perf_${app}.log
for _ in $(seq $trials)
do
    ./build/dry ../../grammars/dblp.cgr ../../datasets/dblp.xml >> perf_${app}.log
done

app=dry_citylots
rm -fv perf_${app}.log
for _ in $(seq $trials)
do
    ./build/dry ../../grammars/citylots2.cgr  ../../datasets/citylots.json >> perf_${app}.log
done
