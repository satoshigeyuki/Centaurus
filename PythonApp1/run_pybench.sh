#!/bin/sh

run='numactl --localalloc --cpunodebind=0'
trials=15

for p in 1 2 4 6 8 10 12 14 16
do
    for app in bench_xml bench2
    do
        for _ in $(seq $trials)
        do
            echo $run ${app}.py $p >> perf_${app}.log
        done
    done
done
