#!/usr/bin/env bash
set -euo pipefail
DATASET=${1:?Usage: ./run_ddfc2000.sh DATASET [METHOD ...]}
shift
if (($# == 0)); then set -- baseline standard gini gini_continuous gini_hybrid; fi
for method in "$@"; do
  common=(--dataset "$DATASET" --folds 10 --runs 10 --seed 1 --generations 2000 --chromosomes 500 --mutation-method standard --crossover standard --local none --fitness class --results-dir "results_ddfc2000/$method")
  case "$method" in
    baseline) ./runmodel.sh "${common[@]}" --mode standard ;;
    standard|gini|gini_continuous|gini_hybrid) ./runmodel.sh "${common[@]}" --mode feature_construction --fc-method "$method" --features 5 --fc-thresholds 5 ;;
    *) echo "Unknown method: $method" >&2; exit 2 ;;
  esac
done
