WORKERS=(1 2 4 6 8 10 12 14 16)
for n in ${WORKERS[@]}
do
    numactl --localalloc --cpunodebind=0 build/bench1 $n >> perf1.log
    numactl --localalloc --cpunodebind=0 build/bench2 $n >> perf2.log
done
