#!/bin/bash

nohup ./runmodel.sh \
  --dataset "$1" \
  --folds 10 \
  --runs 10 \
  --generations 2000 \
  --chromosomes 500 \
  --mutation-method entropy \
  --mutation-rate 0.05 \
  --local none \
  --results-dir results_entropy \
  > "${1}_entropy.log" 2>&1 < /dev/null &
