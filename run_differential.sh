#!/bin/bash
set -eu
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 DATASET" >&2
    exit 2
fi
cd -- "$(dirname -- "$0")"
dataset=$1
nohup ./runmodel.sh \
  --dataset "$dataset" \
  --folds 10 --runs 10 \
  --generations 2000 --chromosomes 500 \
  --mutation-method differential \
  --differential-f 0.5 \
  --differential-fraction 0.5 \
  --mutation-rate 0.05 \
  --local none \
  --results-dir results_differential \
  > "${dataset}_differential.log" 2>&1 < /dev/null &
echo "Started $dataset (PID $!). Log: ${dataset}_differential.log"
