#!/bin/bash

nohup ./runmodel.sh \
  --dataset "$1" \
  --folds 10 --runs 10 \
  --generations 2000 \
  --chromosomes 500 \
  --mutation-method standard \
  --mutation-rate 0.0 \
  --local none \
  --results-dir results_baseline \
  > "${1}_baseline.log" 2>&1 < /dev/null &
