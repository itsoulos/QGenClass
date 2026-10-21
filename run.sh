#!/bin/bash

./runmodel.sh \
    --dataset "$1" \
    --mode feature_construction \
    --features "${2:-1}" \
    --crossitems 0 \
    --runs "${3:-10}"
