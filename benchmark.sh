#!/bin/bash

RESULT_DIR="result"

rm -rf "$RESULT_DIR"
mkdir -p "$RESULT_DIR"

concurrent_requests=(10000 100000 1000000)
parallel_connections=(10 100 1000)

for concurrent in "${concurrent_requests[@]}"; do
    for parallel in "${parallel_connections[@]}"; do
        filename="$RESULT_DIR/result_${concurrent}_${parallel}.txt"
        echo "Running benchmark for $concurrent concurrent requests and $parallel parallel connections..."
        redis-benchmark -d 512 -r 1024 -p 9001 -c $parallel -n $concurrent -t SET,GET >>"$filename"
        echo "Results saved to $filename"
    done
done
