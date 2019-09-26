#!/bin/sh

trials=15

if [ $# -eq 0 ]
then
    apps='xml_load json_load'
else
    apps=$@
fi

for app in $apps
do
    rm -fv perf_${app}.log
    for _ in $(seq $trials)
    do
        ./$app.py
    done  > perf_${app}.log
done
