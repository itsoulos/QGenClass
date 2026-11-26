#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
if [[ $# != 1 || ! $1 =~ ^[a-zA-Z0-9_.-]+$ ]]; then
    echo "Usage: $0 DATASET" >&2; exit 1
fi
dataset=$1
if [[ ! -x ./runmodel.sh ]]; then echo 'runmodel.sh missing or not executable' >&2; exit 1; fi
if [[ ${DIFF_WORKER:-0} != 1 ]]; then
    nohup env DIFF_WORKER=1 bash "$0" "$dataset" > "${dataset}_differential_next.log" 2>&1 < /dev/null &
    echo "Started $dataset (PID $!). Log: ${dataset}_differential_next.log"
    exit 0
fi
for spec in '0.5 0.25 050 025' '0.5 0.75 050 075' '0.25 0.5 025 050' '1.0 0.5 100 050'; do
    read -r f fraction ft pt <<< "$spec"
    tag="diff_f${ft}_p${pt}"
    echo "[$(date -Is)] Starting $dataset F=$f fraction=$fraction"
    ./runmodel.sh --dataset "$dataset" \
        --folds 10 --runs 10 --seed 1 \
        --generations 2000 --chromosomes 500 \
        --mutation-method differential \
        --differential-f "$f" --differential-fraction "$fraction" \
        --mutation-rate 0.05 --local none \
        --results-dir "results_${tag}" \
        > "${dataset}_${tag}.log" 2>&1
    echo "[$(date -Is)] Finished $dataset $tag"
done
echo "[$(date -Is)] All four experiments finished for $dataset"
