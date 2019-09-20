#!/bin/sh

trials=15

rm -fv perf_dblp_stage1.log perf_dblp_dry.log
for _ in $(seq $trials)
do
    ./build/dry ../../grammars/dblp.cgr ../../datasets/dblp.xml >> perf_dblp_stage1.log
    ./build/dblp_opt 1 dry >> perf_dblp_dry.log
done

rm -fv perf_citylots_stage1.log perf_citylots_dry.log
for _ in $(seq $trials)
do
    ./build/dry ../../grammars/citylots2.cgr  ../../datasets/citylots.json >> perf_citylots_stage1.log
    ./build/citylots_opt2 1 dry >> perf_citylots_dry.log
done
