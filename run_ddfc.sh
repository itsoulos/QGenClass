#!/bin/bash

set -e

DATASET="${1:?Usage: ./run_ddfc.sh DATASET}"

COMMON=(
    --dataset "$DATASET"
    --folds 10
    --runs 10
    --seed 1
    --generations 500
    --chromosomes 500
    --mutation-method standard
    --crossover standard
    --local none
    --fitness class
)

echo "===== $DATASET : BASELINE ====="

./runmodel.sh "${COMMON[@]}" \
    --mode standard \
    --results-dir results_ddfc/baseline

echo "===== $DATASET : STANDARD FC ====="

./runmodel.sh "${COMMON[@]}" \
    --mode feature_construction \
    --fc-method standard \
    --features 5 \
    --results-dir results_ddfc/standard

echo "===== $DATASET : GINI FC ====="

./runmodel.sh "${COMMON[@]}" \
    --mode feature_construction \
    --fc-method gini \
    --fc-thresholds 5 \
    --features 5 \
    --results-dir results_ddfc/gini

echo "===== $DATASET : COMPLETED ====="
