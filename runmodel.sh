#!/usr/bin/env bash
set -uo pipefail

usage() {
  cat <<'HELP'
Usage: ./runmodel.sh --dataset NAME [options]
       ./runmodel.sh --train FILE --test FILE [options]

General:
  --exe PATH                  Executable (default ./QGenClass)
  --data-dir DIR              Directory containing NAME.train / NAME.test
  --dataset NAME              Dataset name
  --train FILE                Training file
  --test FILE                 Test file
  --runs N                    Number of runs (default 1)
  --seed N                    First random seed (default 1)
  --results-dir DIR           Logs directory (default results)
  --summary-file FILE         Per-run metrics TSV (default results/summary_*.tsv)
  --help                      Show this help

QGenClass parameters:
  --mode                       default standard  (QGenClass: --mode)
  --features                   default 5  (QGenClass: --features)
  --train-format               default data  (QGenClass: --train_format)
  --test-format                default data  (QGenClass: --test_format)
  --chromosomes                default 500  (QGenClass: --pop_count)
  --length                     default 200  (QGenClass: --pop_size)
  --generations                default 2000  (QGenClass: --pop_gens)
  --selection-rate             default 0.1  (QGenClass: --pop_srate)
  --mutation-rate              default 0.05  (QGenClass: --pop_mrate)
  --local                      default targeted  (QGenClass: --pop_lmethod)
  --fitness                    default class  (QGenClass: --pop_fitnessmethod)
  --termination                default maxiters  (QGenClass: --pop_termination)
  --crossitems                 default 10  (QGenClass: --pop_crossitems)
  --localitems                 default 20  (QGenClass: --pop_localitems)
  --localgens                  default 50  (QGenClass: --pop_localgens)
  --class-percent              default 0.5  (QGenClass: --pop_classpercent)
  --average-percent            default 0.5  (QGenClass: --pop_averagepercent)
  --squared-percent            default 0.0  (QGenClass: --pop_squaredpercent)
  --targeted-iterations        default 50  (QGenClass: --targeted_iterations)
  --targeted-stagnation-limit  default 20  (QGenClass: --targeted_stagnationlimit)
  --targeted-burst-iterations  default 200  (QGenClass: --targeted_burstiterations)
  --crossover                  default targeted  (QGenClass: --cross_method)
  --targeted-cross-iterations  default 20  (QGenClass: --targeted_crossiterations)
  --targeted-cross-elite-fraction default 0.2  (QGenClass: --targeted_crosselitefraction)
  --targeted-cross-max-block   default 16  (QGenClass: --targeted_crossmaxblock)
  --smote                      default no  (QGenClass: --enable_smote)
  --smote-k                    default 5  (QGenClass: --smote_k)
  --c-output                   default classifier.c  (QGenClass: --coutput_file)
  --python-output              default classifier.py  (QGenClass: --pythonoutput_file)

Aliases: --population for --chromosomes; --local-method for --local
         --fitness-method for --fitness; --cross-method for --crossover
         --feature-construction is shorthand for --mode feature_construction
HELP
}

exe=./QGenClass
data_dir="$HOME/Desktop/ERGASIES/FeatureConstruction2/datasets/tenfolding"
dataset=; train=; test=; runs=1; seed=1; results_dir=results; summary_file=
declare -A config=()
config[mode]='standard'
config[features]='5'
config[train_format]='data'
config[test_format]='data'
config[pop_count]='500'
config[pop_size]='200'
config[pop_gens]='2000'
config[pop_srate]='0.1'
config[pop_mrate]='0.05'
config[pop_lmethod]='targeted'
config[pop_fitnessmethod]='class'
config[pop_termination]='maxiters'
config[pop_crossitems]='10'
config[pop_localitems]='20'
config[pop_localgens]='50'
config[pop_classpercent]='0.5'
config[pop_averagepercent]='0.5'
config[pop_squaredpercent]='0.0'
config[targeted_iterations]='50'
config[targeted_stagnationlimit]='20'
config[targeted_burstiterations]='200'
config[cross_method]='targeted'
config[targeted_crossiterations]='20'
config[targeted_crosselitefraction]='0.2'
config[targeted_crossmaxblock]='16'
config[enable_smote]='no'
config[smote_k]='5'
config[coutput_file]='classifier.c'
config[pythonoutput_file]='classifier.py'

declare -A options=()
options[mode]=mode
options[features]=features
options[train-format]=train_format
options[test-format]=test_format
options[chromosomes]=pop_count
options[length]=pop_size
options[generations]=pop_gens
options[selection-rate]=pop_srate
options[mutation-rate]=pop_mrate
options[local]=pop_lmethod
options[fitness]=pop_fitnessmethod
options[termination]=pop_termination
options[crossitems]=pop_crossitems
options[localitems]=pop_localitems
options[localgens]=pop_localgens
options[class-percent]=pop_classpercent
options[average-percent]=pop_averagepercent
options[squared-percent]=pop_squaredpercent
options[targeted-iterations]=targeted_iterations
options[targeted-stagnation-limit]=targeted_stagnationlimit
options[targeted-burst-iterations]=targeted_burstiterations
options[crossover]=cross_method
options[targeted-cross-iterations]=targeted_crossiterations
options[targeted-cross-elite-fraction]=targeted_crosselitefraction
options[targeted-cross-max-block]=targeted_crossmaxblock
options[smote]=enable_smote
options[smote-k]=smote_k
options[c-output]=coutput_file
options[python-output]=pythonoutput_file
options[population]=pop_count
options[local-method]=pop_lmethod
options[fitness-method]=pop_fitnessmethod
options[cross-method]=cross_method

while (($#)); do
  case "$1" in
    --help|-h) usage; exit 0 ;;
    --feature-construction) config[mode]=feature_construction; shift ;;
    --*)
      key=${1#--}
      if [[ "$key" == *=* ]]; then value=${key#*=}; key=${key%%=*}; shift
      else
        if (($# < 2)); then echo "Missing value for --$key" >&2; exit 2; fi
        value=$2; shift 2
      fi
      case "$key" in
        exe|data-dir|dataset|train|test|runs|seed|results-dir|summary-file)
          case "$key" in
            exe) exe=$value ;; data-dir) data_dir=$value ;; dataset) dataset=$value ;;
            train) train=$value ;; test) test=$value ;; runs) runs=$value ;;
            seed) seed=$value ;; results-dir) results_dir=$value ;;
            summary-file) summary_file=$value ;;
          esac ;;
        *) if [[ -v options[$key] ]]; then config[${options[$key]}]=$value
           else echo "Unknown option: --$key" >&2; usage >&2; exit 2; fi ;;
      esac ;;
    *) echo "Unexpected argument: $1" >&2; usage >&2; exit 2 ;;
  esac
done

for v in runs seed; do
  if [[ ! ${!v} =~ ^[0-9]+$ ]]; then echo "Invalid $v: ${!v}" >&2; exit 2; fi
done
(( runs > 0 )) || { echo 'runs must be positive' >&2; exit 2; }
(( seed + runs - 1 <= 1000 )) || { echo 'QGenClass random_seed range is 0..1000' >&2; exit 2; }
case ${config[mode]} in standard|feature_construction) ;; *) echo 'Invalid mode' >&2; exit 2;; esac
case ${config[pop_lmethod]} in none|crossover|mutate|mutateWorst|siman|hill|de|gd|adam|targeted|targetedWorst|targetedBest|constants) ;; *) echo 'Invalid local search' >&2; exit 2;; esac
case ${config[pop_fitnessmethod]} in class|average|squared|mixed|mean|weightedF1|macroF1) ;; *) echo 'Invalid fitness' >&2; exit 2;; esac
case ${config[cross_method]} in standard|targeted|targetedWorst) ;; *) echo 'Invalid crossover' >&2; exit 2;; esac
case ${config[pop_termination]} in maxiters|doublebox|similarity) ;; *) echo 'Invalid termination' >&2; exit 2;; esac
case ${config[enable_smote]} in yes|no) ;; *) echo 'Invalid smote value' >&2; exit 2;; esac
for fmt in train_format test_format; do case ${config[$fmt]} in data|csv|arff) ;; *) echo "Invalid $fmt" >&2; exit 2;; esac; done
if [[ ! ${config[features]} =~ ^[0-9]+$ ]] || ((config[features] < 1 || config[features] > 100)); then
  echo 'features must be between 1 and 100' >&2; exit 2
fi
if [[ -n $dataset ]]; then
  [[ -n $train ]] || train="$data_dir/$dataset.train"
  [[ -n $test ]] || test="$data_dir/$dataset.test"
fi
[[ -n $train && -n $test ]] || { echo 'Specify --dataset or --train and --test' >&2; exit 2; }
for v in train test; do
  if [[ ! -f ${!v} && -f $data_dir/${!v} ]]; then printf -v "$v" '%s' "$data_dir/${!v}"; fi
  [[ -f ${!v} ]] || { echo "Missing $v file: ${!v}" >&2; exit 2; }
done
[[ -x $exe ]] || { echo "Missing executable: $exe" >&2; exit 2; }
mkdir -p -- "$results_dir" || exit 2
dataset_tag=${dataset:-$(basename "$train")}
dataset_tag=${dataset_tag//[^a-zA-Z0-9_.-]/_}
if [[ -z $summary_file ]]; then
  summary_file="$results_dir/${dataset_tag}_${config[mode]}_${config[pop_fitnessmethod]}_summary_$(date +%Y%m%d_%H%M%S).tsv"
fi
mkdir -p -- "$(dirname "$summary_file")"
printf 'run\tseed\tclass_error_percent\taccuracy_percent\tprecision\trecall\tf1score\tlogfile\n' > "$summary_file"
# Sort keys to ensure reproducible argument order.
mapfile -t keys < <(printf '%s\n' "${!config[@]}" | LC_ALL=C sort)
for ((run=0;run<runs;run++)); do
  current_seed=$((seed+run))
  cmd=("$exe" "--train_file=$train" "--test_file=$test" "--random_seed=$current_seed")
  for key in "${keys[@]}"; do cmd+=("--$key=${config[$key]}"); done
  logfile="$results_dir/${dataset_tag}_${config[mode]}_${config[pop_fitnessmethod]}_seed${current_seed}_$(date +%Y%m%d_%H%M%S).log"
  printf 'Running (%d/%d): ' "$((run+1))" "$runs"; printf '%q ' "${cmd[@]}"; echo
  "${cmd[@]}" 2>&1 | tee "$logfile"
  status=${PIPESTATUS[0]}
  if ((status != 0)); then echo "QGenClass failed (exit $status); log: $logfile" >&2; exit "$status"; fi
  metrics=$(LC_ALL=C awk '
    /CLASS_ERROR:/ {
      ce=pr=re=f1=""
      for(i=1;i<=NF;i++) {
        if($i=="CLASS_ERROR:") ce=$(i+1)
        else if($i=="PRECISION:") pr=$(i+1)
        else if($i=="RECALL:") re=$(i+1)
        else if($i=="F1SCORE:") f1=$(i+1)
      }
      if(ce!="" && pr!="" && re!="" && f1!="")
        result=ce " " pr " " re " " f1
    }
    END {print result}
  ' "$logfile")
  if [[ -z $metrics ]]; then
    echo "Missing CLASS_ERROR/PRECISION/RECALL/F1SCORE in $logfile" >&2
    exit 1
  fi
  read -r class_error precision recall f1score <<< "$metrics"
  accuracy=$(awk -v e="$class_error" 'BEGIN {printf "%.10f",100-e}')
  printf '%d\t%d\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$((run+1))" "$current_seed" "$class_error" "$accuracy" \
    "$precision" "$recall" "$f1score" "$logfile" >> "$summary_file"
  echo "Log: $logfile"
done

echo
echo "Summary for $runs runs:"
LC_ALL=C awk -F '\t' '
NR==1 {next}
{
  n++
  for(i=3;i<=7;i++) {
    x=$i+0
    sum[i]+=x
    sq[i]+=x*x
  }
}
END {
  if(n==0) exit 1
  name[3]="Class error (%)"
  name[4]="Accuracy (%)"
  name[5]="Precision"
  name[6]="Recall"
  name[7]="F1 score"
  printf "%-19s %14s %14s\n","Metric","Mean","Std.Dev."
  for(i=3;i<=7;i++) {
    avg=sum[i]/n
    variance=n>1?(sq[i]-sum[i]*sum[i]/n)/(n-1):0
    if(variance<0) variance=0
    printf "%-19s %14.8f %14.8f\n",name[i],avg,sqrt(variance)
  }
}' "$summary_file"
echo "Per-run results: $summary_file"

