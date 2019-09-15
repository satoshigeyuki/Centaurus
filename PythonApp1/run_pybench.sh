#!/bin/sh

run='numactl --localalloc --cpunodebind=0'
trials=15

if [ $# -eq 0 ]
then
    apps='citylots_opt2 dblp_opt'
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
            $run ./${app}.py $p >> perf_${app}.log
        done
    done
done
for p in $(seq 18 2 34)
do
    for app in ${apps}
    do
        for _ in $(seq $trials)
        do
            ./${app}.py $p >> perf_${app}.log
        done
    done
done
