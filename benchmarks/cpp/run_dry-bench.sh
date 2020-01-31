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
    rm -fv perf_${app}_stage1.log perf_${app}_dry.log
    case $app in
        'citylots') suffix='.json' ;;
        'dblp') suffix='.xml' ;;
        *) echo 'unexpected app:' $app; exit ;;
    esac
    for _ in $(seq $trials)
    do
        ./build/dry ../../grammars/${app}.cgr ../../datasets/${app}${suffix} >> perf_${app}_stage1.log
        ./build/${app} 1 dry >> perf_${app}_dry.log
    done
done
