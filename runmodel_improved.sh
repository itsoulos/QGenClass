#!/usr/bin/env bash
set -o pipefail
DATAPATH="${DATAPATH:-$HOME/Desktop/ERGASIES/FeatureConstruction2/datasets/tenfolding}"
PROGRAM="${PROGRAM:-./QGenClass}"
RESULTSDIR="${RESULTSDIR:-results}"
DATAFILE="${1:-}"
SEED="${2:-1}"
LOCAL_METHOD="${3:-targeted}"
FITNESS_METHOD="${4:-class}"
if [[ -z "$DATAFILE" ]]; then
  echo "Usage: $0 DATASET [SEED] [LOCAL_METHOD] [FITNESS_METHOD]"
  echo "Example: $0 ionosphere 12345 targeted class"
  exit 1
fi
if ! [[ "$SEED" =~ ^[0-9]+$ ]]; then echo "ERROR: SEED must be a non-negative integer."; exit 1; fi
case "$LOCAL_METHOD" in
  none|crossover|mutate|siman|hill|de|gd|adam|mutateWorst|targeted|targetedWorst|targetedBest|constants) ;;
  *) echo "ERROR: Unknown local method: $LOCAL_METHOD"; exit 1;;
esac
case "$FITNESS_METHOD" in
  class|average|squared|mixed|mean|macroF1|weightedF1) ;;
  *) echo "ERROR: Unknown fitness method: $FITNESS_METHOD"; exit 1;;
esac
TRAIN_FILE="$DATAPATH/$DATAFILE.train"
TEST_FILE="$DATAPATH/$DATAFILE.test"
[[ -x "$PROGRAM" ]] || { echo "ERROR: executable not found: $PROGRAM"; exit 1; }
[[ -f "$TRAIN_FILE" ]] || { echo "ERROR: training file not found: $TRAIN_FILE"; exit 1; }
[[ -f "$TEST_FILE" ]] || { echo "ERROR: test file not found: $TEST_FILE"; exit 1; }
POP_COUNT="${POP_COUNT:-500}"
POP_SIZE="${POP_SIZE:-200}"
POP_GENS="${POP_GENS:-2000}"
POP_SRATE="${POP_SRATE:-0.1}"
POP_MRATE="${POP_MRATE:-0.05}"
POP_CROSSITEMS="${POP_CROSSITEMS:-0}"
POP_LOCALITEMS="${POP_LOCALITEMS:-20}"
POP_LOCALGENS="${POP_LOCALGENS:-50}"
POP_CLASSPERCENT="${POP_CLASSPERCENT:-0.50}"
POP_AVERAGEPERCENT="${POP_AVERAGEPERCENT:-0.50}"
POP_SQUAREDPERCENT="${POP_SQUAREDPERCENT:-0.00}"
TARGETED_ITERATIONS="${TARGETED_ITERATIONS:-50}"
TARGETED_STAGNATION_LIMIT="${TARGETED_STAGNATION_LIMIT:-20}"
TARGETED_BURST_ITERATIONS="${TARGETED_BURST_ITERATIONS:-200}"
TRAIN_FORMAT="${TRAIN_FORMAT:-data}"
TEST_FORMAT="${TEST_FORMAT:-data}"
ENABLE_SMOTE="${ENABLE_SMOTE:-no}"
SMOTE_K="${SMOTE_K:-5}"
mkdir -p "$RESULTSDIR"
TIMESTAMP="$(date '+%Y%m%d_%H%M%S')"
LOGFILE="$RESULTSDIR/${DATAFILE}_${LOCAL_METHOD}_${FITNESS_METHOD}_seed${SEED}_${TIMESTAMP}.log"
CMD=(
  "$PROGRAM"
  "--pop_count=$POP_COUNT"
  "--pop_size=$POP_SIZE"
  "--pop_gens=$POP_GENS"
  "--pop_srate=$POP_SRATE"
  "--pop_mrate=$POP_MRATE"
  "--pop_lmethod=$LOCAL_METHOD"
  "--pop_fitnessmethod=$FITNESS_METHOD"
  "--pop_crossitems=$POP_CROSSITEMS"
  "--pop_localitems=$POP_LOCALITEMS"
  "--pop_localgens=$POP_LOCALGENS"
  "--pop_classpercent=$POP_CLASSPERCENT"
  "--pop_averagepercent=$POP_AVERAGEPERCENT"
  "--pop_squaredpercent=$POP_SQUAREDPERCENT"
  "--targeted_iterations=$TARGETED_ITERATIONS"
  "--targeted_stagnationlimit=$TARGETED_STAGNATION_LIMIT"
  "--targeted_burstiterations=$TARGETED_BURST_ITERATIONS"
  "--random_seed=$SEED"
  "--train_file=$TRAIN_FILE"
  "--train_format=$TRAIN_FORMAT"
  "--test_file=$TEST_FILE"
  "--test_format=$TEST_FORMAT"
  "--enable_smote=$ENABLE_SMOTE"
  "--smote_k=$SMOTE_K"
)
echo
echo "=============================================================================="
echo " QGenClass experiment"
echo "=============================================================================="
echo " Dataset             : $DATAFILE"
echo " Seed                : $SEED"
echo " Local method        : $LOCAL_METHOD"
echo " Fitness method      : $FITNESS_METHOD"
echo " Population          : $POP_COUNT"
echo " Chromosome size     : $POP_SIZE"
echo " Generations         : $POP_GENS"
echo " Local items         : $POP_LOCALITEMS"
echo " Local interval      : $POP_LOCALGENS"
echo " Targeted iterations : $TARGETED_ITERATIONS"
echo " Stagnation limit    : $TARGETED_STAGNATION_LIMIT"
echo " Burst iterations    : $TARGETED_BURST_ITERATIONS"
echo " Train file          : $TRAIN_FILE"
echo " Test file           : $TEST_FILE"
echo " Log file            : $LOGFILE"
echo "=============================================================================="
echo
printf ' %q' "${CMD[@]}"; echo; echo
START_TIME="$(date +%s)"
if command -v stdbuf >/dev/null 2>&1; then
  stdbuf -oL -eL "${CMD[@]}" 2>&1 | tee "$LOGFILE"
else
  "${CMD[@]}" 2>&1 | tee "$LOGFILE"
fi
PROGRAM_STATUS="${PIPESTATUS[0]}"
END_TIME="$(date +%s)"
ELAPSED=$((END_TIME - START_TIME))
HOURS=$((ELAPSED / 3600))
MINUTES=$(((ELAPSED % 3600) / 60))
SECONDS=$((ELAPSED % 60))
echo
echo "=============================================================================="
if [[ "$PROGRAM_STATUS" -eq 0 ]]; then echo " Experiment completed successfully."; else echo " Experiment FAILED with exit code $PROGRAM_STATUS."; fi
printf " Execution time      : %02d:%02d:%02d\n" "$HOURS" "$MINUTES" "$SECONDS"
echo " Log file            : $LOGFILE"
echo "=============================================================================="
exit "$PROGRAM_STATUS"
