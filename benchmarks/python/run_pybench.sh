#!/bin/sh

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

run='numactl --localalloc --cpunodebind=0'
for p in 1 $(seq 2 2 22)
do
    for app in ${apps}
    do
        for _ in $(seq $trials)
        do
            $run ./${app}.py $p >> perf_${app}.log
        done
    done
done
run='numactl --localalloc --cpunodebind=0-1'
for p in $(seq 24 2 46)
do
    for app in ${apps}
    do
        for _ in $(seq $trials)
        do
            $run ./${app}.py $p >> perf_${app}.log
        done
    done
done
run='numactl --localalloc --cpunodebind=0-2'
for p in $(seq 48 2 70)
do
    for app in ${apps}
    do
        for _ in $(seq $trials)
        do
            $run ./${app}.py $p >> perf_${app}.log
        done
    done
done
