#!/bin/sh

run='numactl --localalloc --cpunodebind=0'
trials=15

apps='bench_xml bench2'

for app in ${apps}
do
    rm -fv ${app}.py
done
for p in 1 2 4 6 8 10 12 14 16
do
    for app in ${apps}
    do
        for _ in $(seq $trials)
        do
            $run ${app}.py $p >> perf_${app}.log
        done
    done
done
