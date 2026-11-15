#!/usr/bin/env bash
# Usage: ./monitor DATASET
set -euo pipefail
if [[ $# -ne 1 || "$1" == --help || "$1" == -h ]]; then
  echo 'Usage: ./monitor DATASET  (e.g. ./monitor ionosphere)'
  exit 2
fi
cd -- "$(dirname -- "$(readlink -f -- "$0")")"
exec python3 ./monitor_qgenclass.py --dataset "$1"
