#!/bin/sh

run='numactl --localalloc --cpunodebind=0'
trials=15

if [ $# -eq 0 ]
then
    apps='citylots dblp'
else
    apps=$@
fi

for app in ${apps}
do
    rm -fv perf_${app}.log
done

for p in 1 $(seq 2 2 16)
do
    for app in ${apps}
    do
        for _ in $(seq $trials)
        do
            $run ./build/${app} $p >> perf_${app}.log
        done
    done
done

for p in $(seq 18 2 34)
do
    for app in ${apps}
    do
        for _ in $(seq $trials)
        do
            ./build/${app} $p >> perf_${app}.log
        done
    done
done
