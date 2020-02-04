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

run_apps() {
    for p in $*
    do
        for app in ${apps}
        do
            for _ in $(seq $trials)
            do
                $run ./${app}.py $p >> perf_${app}.log
            done
        done
    done
}

run='numactl --localalloc'
run_apps 1 $(seq 2 2 94)
