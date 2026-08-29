#!/usr/bin/env bash

set -o pipefail

# ==============================================================================
# QGenClass repeated experiment runner
#
# Usage:
#   ./runmodel_30.sh DATASET [REPETITIONS] [BASE_SEED] [LOCAL_METHOD] [FITNESS_METHOD]
#
# Examples:
#   ./runmodel_30.sh ionosphere_1
#   ./runmodel_30.sh ionosphere_1 30 1 targeted class
#   ./runmodel_30.sh ionosphere_1 30 100 targetedWorst mean
#   ./runmodel_30.sh ionosphere_1 30 1 constants macroF1
#
# Defaults:
#   REPETITIONS   = 30
#   BASE_SEED     = 1
#   LOCAL_METHOD  = targeted
#   FITNESS       = class
# ==============================================================================

DATAPATH="${DATAPATH:-$HOME/Desktop/ERGASIES/FeatureConstruction2/datasets/tenfolding}"
PROGRAM="${PROGRAM:-./QGenClass}"
RESULTSDIR="${RESULTSDIR:-results}"

DATAFILE="${1:-}"
REPETITIONS="${2:-30}"
BASE_SEED="${3:-1}"
LOCAL_METHOD="${4:-targeted}"
FITNESS_METHOD="${5:-class}"

if [[ -z "$DATAFILE" ]]; then
    echo "Usage:"
    echo "  $0 DATASET [REPETITIONS] [BASE_SEED] [LOCAL_METHOD] [FITNESS_METHOD]"
    echo
    echo "Example:"
    echo "  $0 ionosphere_1 30 1 targeted class"
    exit 1
fi

if ! [[ "$REPETITIONS" =~ ^[1-9][0-9]*$ ]]; then
    echo "ERROR: REPETITIONS must be a positive integer."
    exit 1
fi

if ! [[ "$BASE_SEED" =~ ^[0-9]+$ ]]; then
    echo "ERROR: BASE_SEED must be a non-negative integer."
    exit 1
fi

case "$LOCAL_METHOD" in
    none|crossover|mutate|siman|hill|de|gd|adam|mutateWorst|targeted|targetedWorst|targetedBest|constants)
        ;;
    *)
        echo "ERROR: Unknown local method: $LOCAL_METHOD"
        echo "Available:"
        echo "  none crossover mutate siman hill de gd adam mutateWorst"
        echo "  targeted targetedWorst targetedBest constants"
        exit 1
        ;;
esac

case "$FITNESS_METHOD" in
    class|average|squared|mixed|mean|macroF1|weightedF1)
        ;;
    *)
        echo "ERROR: Unknown fitness method: $FITNESS_METHOD"
        echo "Available:"
        echo "  class average squared mixed mean macroF1 weightedF1"
        exit 1
        ;;
esac

TRAIN_FILE="$DATAPATH/$DATAFILE.train"
TEST_FILE="$DATAPATH/$DATAFILE.test"

if [[ ! -x "$PROGRAM" ]]; then
    echo "ERROR: executable not found or not executable:"
    echo "  $PROGRAM"
    exit 1
fi

if [[ ! -f "$TRAIN_FILE" ]]; then
    echo "ERROR: training file not found:"
    echo "  $TRAIN_FILE"
    exit 1
fi

if [[ ! -f "$TEST_FILE" ]]; then
    echo "ERROR: test file not found:"
    echo "  $TEST_FILE"
    exit 1
fi

# ==============================================================================
# Main QGenClass parameters
# ==============================================================================

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

TAG="${DATAFILE}_${LOCAL_METHOD}_${FITNESS_METHOD}"

RESULTS_FILE="$RESULTSDIR/results_${TAG}.txt"
SUMMARY_FILE="$RESULTSDIR/summary_${TAG}.txt"

rm -f "$RESULTS_FILE" "$SUMMARY_FILE"

echo "# Run Seed ClassError Accuracy Precision Recall F1Score TimeSeconds LogFile" > "$RESULTS_FILE"

echo
echo "================================================================================"
echo " QGenClass repeated experiment"
echo "================================================================================"
echo " Dataset             : $DATAFILE"
echo " Repetitions         : $REPETITIONS"
echo " Base seed           : $BASE_SEED"
echo " Local method        : $LOCAL_METHOD"
echo " Fitness method      : $FITNESS_METHOD"
echo " Population          : $POP_COUNT"
echo " Chromosome size     : $POP_SIZE"
echo " Generations         : $POP_GENS"
echo " Mutation rate       : $POP_MRATE"
echo " Selection rate      : $POP_SRATE"
echo " Local items         : $POP_LOCALITEMS"
echo " Local interval      : $POP_LOCALGENS"
echo " Targeted iterations : $TARGETED_ITERATIONS"
echo " Stagnation limit    : $TARGETED_STAGNATION_LIMIT"
echo " Burst iterations    : $TARGETED_BURST_ITERATIONS"
echo " Results file        : $RESULTS_FILE"
echo " Summary file        : $SUMMARY_FILE"
echo "================================================================================"
echo

TOTAL_START="$(date +%s)"

for ((RUN=1; RUN<=REPETITIONS; RUN++)); do

    SEED=$((BASE_SEED + RUN - 1))
    LOGFILE="$RESULTSDIR/run_${TAG}_seed${SEED}.log"

    echo
    echo "################################################################################"
    echo "# RUN $RUN / $REPETITIONS"
    echo "# SEED = $SEED"
    echo "################################################################################"
    echo

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

    START_TIME="$(date +%s)"

    echo "[RUN $RUN] Starting..."
    echo "[RUN $RUN] Log: $LOGFILE"
    echo

    if command -v stdbuf >/dev/null 2>&1; then
        stdbuf -oL -eL "${CMD[@]}" 2>&1 |
        tee "$LOGFILE" |
        awk -v run="$RUN" '
            /^GENERATION=/ {
                print "[RUN " run "] " $0
                fflush()
            }

            /^TARGETED\[/ {
                if ($0 ~ /END/) {
                    print "[RUN " run "] " $0
                    fflush()
                }
            }

            /^TARGETED_WORST\[/ {
                if ($0 ~ /END/) {
                    print "[RUN " run "] " $0
                    fflush()
                }
            }

            /^TARGETED_BEST\[/ {
                if ($0 ~ /END/) {
                    print "[RUN " run "] " $0
                    fflush()
                }
            }

            /^CONSTANTS\[/ {
                if ($0 ~ /END/) {
                    print "[RUN " run "] " $0
                    fflush()
                }
            }

            /^CLASS_ERROR:/ {
                print "[RUN " run "] FINAL: " $0
                fflush()
            }

            /^ERROR:/ {
                print "[RUN " run "] " $0
                fflush()
            }
        '
    else
        "${CMD[@]}" 2>&1 |
        tee "$LOGFILE" |
        awk -v run="$RUN" '
            /^GENERATION=/ {
                print "[RUN " run "] " $0
                fflush()
            }

            /^CLASS_ERROR:/ {
                print "[RUN " run "] FINAL: " $0
                fflush()
            }

            /^ERROR:/ {
                print "[RUN " run "] " $0
                fflush()
            }
        '
    fi

    PIPE_VALUES=("${PIPESTATUS[@]}")
    PROGRAM_STATUS="${PIPE_VALUES[0]}"

    if [[ "$PROGRAM_STATUS" -ne 0 ]]; then
        echo
        echo "[RUN $RUN] ERROR: QGenClass failed with status $PROGRAM_STATUS"
        echo "[RUN $RUN] See: $LOGFILE"
        exit "$PROGRAM_STATUS"
    fi

    END_TIME="$(date +%s)"
    ELAPSED=$((END_TIME - START_TIME))

    # --------------------------------------------------------------------------
    # Parse the final QGenClass metric line.
    #
    # Example:
    #
    # CLASS_ERROR: 11.4285714286 PRECISION: 0.9130434783
    # RECALL: 0.8750000000 F1SCORE: 0.8936170213
    # --------------------------------------------------------------------------

    METRIC_LINE="$(
        grep '^CLASS_ERROR:' "$LOGFILE" |
        tail -n 1
    )"

    if [[ -z "$METRIC_LINE" ]]; then
        echo
        echo "[RUN $RUN] ERROR: final CLASS_ERROR line was not found."
        echo "[RUN $RUN] See: $LOGFILE"
        exit 1
    fi

    read -r CLASS_ERROR PRECISION RECALL F1SCORE <<< "$(
        echo "$METRIC_LINE" |
        awk '
        {
            for (i=1; i<=NF; i++) {
                if ($i=="CLASS_ERROR:") {
                    class_error=$(i+1)
                }
                else if ($i=="PRECISION:") {
                    precision=$(i+1)
                }
                else if ($i=="RECALL:") {
                    recall=$(i+1)
                }
                else if ($i=="F1SCORE:") {
                    f1=$(i+1)
                }
            }

            print class_error, precision, recall, f1
        }
        '
    )"

    if [[ -z "$CLASS_ERROR" || -z "$PRECISION" || -z "$RECALL" || -z "$F1SCORE" ]]; then
        echo
        echo "[RUN $RUN] ERROR: could not parse final metrics."
        echo "[RUN $RUN] Line:"
        echo "$METRIC_LINE"
        exit 1
    fi

    # Accuracy derived from percentage class error.
    ACCURACY="$(
        awk -v e="$CLASS_ERROR" '
            BEGIN {
                printf "%.10f", 1.0 - e/100.0
            }
        '
    )"

    echo \
"$RUN $SEED $CLASS_ERROR $ACCURACY $PRECISION $RECALL $F1SCORE $ELAPSED $LOGFILE" \
        >> "$RESULTS_FILE"

    HOURS=$((ELAPSED / 3600))
    MINUTES=$(((ELAPSED % 3600) / 60))
    SECONDS=$((ELAPSED % 60))

    echo
    echo "-------------------------------------------------------------------------------"
    echo "[RUN $RUN] RESULTS"
    echo "-------------------------------------------------------------------------------"
    echo " Class error : $CLASS_ERROR %"
    echo " Accuracy    : $ACCURACY"
    echo " Precision   : $PRECISION"
    echo " Recall      : $RECALL"
    echo " F1 score    : $F1SCORE"
    printf " Time        : %02d:%02d:%02d\n" "$HOURS" "$MINUTES" "$SECONDS"
    echo "-------------------------------------------------------------------------------"

done

# ==============================================================================
# Aggregate statistics
# ==============================================================================

awk '
NR > 1 {
    n++

    # 3 = ClassError
    # 4 = Accuracy
    # 5 = Precision
    # 6 = Recall
    # 7 = F1Score
    # 8 = TimeSeconds

    for (i=3; i<=8; i++) {
        x=$i

        sum[i]+=x
        sumsq[i]+=x*x

        if (n==1 || x<minv[i]) {
            minv[i]=x
        }

        if (n==1 || x>maxv[i]) {
            maxv[i]=x
        }
    }
}

END {
    if (n==0) {
        print "ERROR: no results."
        exit
    }

    name[3]="Class Error (%)"
    name[4]="Accuracy"
    name[5]="Precision"
    name[6]="Recall"
    name[7]="F1 Score"
    name[8]="Time (sec)"

    print ""
    print "================================================================================"
    print " FINAL STATISTICS"
    print "================================================================================"
    print ""

    printf "%-20s %14s %14s %14s %14s\n",
           "Metric",
           "Average",
           "Std.Dev.",
           "Minimum",
           "Maximum"

    print "--------------------------------------------------------------------------------"

    for (i=3; i<=8; i++) {
        mean=sum[i]/n

        if (n>1) {
            variance=(sumsq[i]-n*mean*mean)/(n-1)

            if (variance<0) {
                variance=0
            }

            sd=sqrt(variance)
        }
        else {
            sd=0
        }

        printf "%-20s %14.6f %14.6f %14.6f %14.6f\n",
               name[i],
               mean,
               sd,
               minv[i],
               maxv[i]
    }

    print ""
    print "Number of runs =", n
    print "================================================================================"
}
' "$RESULTS_FILE" | tee "$SUMMARY_FILE"

TOTAL_END="$(date +%s)"
TOTAL_ELAPSED=$((TOTAL_END - TOTAL_START))

TOTAL_HOURS=$((TOTAL_ELAPSED / 3600))
TOTAL_MINUTES=$(((TOTAL_ELAPSED % 3600) / 60))
TOTAL_SECONDS=$((TOTAL_ELAPSED % 60))

echo
echo "================================================================================"
echo " ALL RUNS COMPLETED"
echo "================================================================================"
printf " Total execution time : %02d:%02d:%02d\n" \
    "$TOTAL_HOURS" "$TOTAL_MINUTES" "$TOTAL_SECONDS"
echo " Detailed results     : $RESULTS_FILE"
echo " Summary              : $SUMMARY_FILE"
echo "================================================================================"
echo
