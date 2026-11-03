#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C

DIR="${1:-results/ablations_dermatology_1}"
OUT="$DIR/acceptance_analysis"
mkdir -p "$OUT"

methods=(standard standardElite standardBlock targeted targetedFull targetedWorst)
labels=(STANDARD STANDARD_ELITE STANDARD_BLOCK TARGETED TARGETED_FULL TARGETEDWORST)

printf 'method\tseed\tfull_generations\tpartial_generations\ttrials\tcrossover_evals\taccepted\tskipped\taccepted_per_generation\taccepted_per_1000_crossover_evals\n' > "$OUT/per_run.tsv"
printf 'method\tseed\tbin\taccepted\tcrossover_evals\ttrials\tfitness_evals_end\n' > "$OUT/per_run_bins.tsv"

for i in "${!methods[@]}"; do
  method="${methods[i]}"; label="${labels[i]}"
  for seed in $(seq 1 30); do
    f="$DIR/crossover_acceptance_dermatology_1.train_${method}_seed${seed}.tsv"
    if [[ ! -f "$f" ]]; then
      echo "ERROR: Missing $f" >&2; exit 1
    fi
    awk -v method="$method" -v label="$label" -v seed="$seed" \
        -v perrun="$OUT/per_run.tsv" -v bins="$OUT/per_run_bins.tsv" '
      BEGIN { FS="[[:space:]]+"; OFS="\t"; last=0 }
      NR==1 {next}
      $2==label {
        g=$1+0; fe=$3+0; t=$4+0; e=$5+0; a=$6+0; s=$7+0; partial=$8+0
        if (fe<last) { print "ERROR: decreasing fitness evaluations: " FILENAME > "/dev/stderr"; exit 2 }
        last=fe
        trials+=t; ce+=e; accepted+=a; skipped+=s
        if (partial) partial_gens++
        else full_gens++
        # Group each generation by its ending cumulative fitness evaluation count.
        # Fixed 10,000-evaluation bins; zero bins are emitted below.
        b=int((fe-1)/10000)+1
        if (b<1) b=1
        if (b>10) b=10
        ba[b]+=a; be[b]+=e; bt[b]+=t
      }
      END {
        if (last==0) { print "ERROR: no data for " FILENAME > "/dev/stderr"; exit 2 }
        apg=(full_gens>0 ? accepted/full_gens : 0)
        ape=(ce>0 ? 1000*accepted/ce : 0)
        print method,seed,full_gens,partial_gens,trials,ce,accepted,skipped,apg,ape >> perrun
        for (b=1;b<=10;b++) print method,seed,b,ba[b]+0,be[b]+0,bt[b]+0,b*10000 >> bins
      }
    ' "$f"
  done
done

# Means and sample SD, each of the 30 runs is one observation.
awk 'BEGIN{FS=OFS="\t"; print "method","runs","mean_accepted","sd_accepted","mean_accepted_per_generation","sd_accepted_per_generation","mean_accepted_per_1000_crossover_evals","sd_accepted_per_1000_crossover_evals","mean_full_generations","mean_crossover_evals"}
NR==1{next}
{
 m=$1; n[m]++
 x[1]=$7; x[2]=$9; x[3]=$10; x[4]=$3; x[5]=$6
 for(j=1;j<=5;j++){sum[m,j]+=x[j];sq[m,j]+=x[j]*x[j]}
}
END{
 split("standard standardElite standardBlock targeted targetedFull targetedWorst", order," ")
 for(k=1;k<=6;k++){
 m=order[k]; if(n[m]!=30){print "ERROR: "m" has "n[m]" runs" > "/dev/stderr"; exit 2}
 for(j=1;j<=5;j++){
  avg[j]=sum[m,j]/n[m]
  v=(sq[m,j]-sum[m,j]*sum[m,j]/n[m])/(n[m]-1)
  sd[j]=sqrt(v>0?v:0)
 }
 printf "%s\t%d\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\n",m,n[m],avg[1],sd[1],avg[2],sd[2],avg[3],sd[3],avg[4],avg[5]
 }
}' "$OUT/per_run.tsv" > "$OUT/summary.tsv"

awk 'BEGIN{FS=OFS="\t"; print "method","fitness_evals_end","mean_accepted_in_bin","sd_accepted_in_bin","mean_crossover_evals_in_bin","mean_trials_in_bin"}
NR==1{next}
{
 key=$1 SUBSEP $3; n[key]++; x=$4; sx[key]+=x; sq[key]+=x*x; se[key]+=$5; st[key]+=$6
}
END{
 split("standard standardElite standardBlock targeted targetedFull targetedWorst", order," ")
 for(k=1;k<=6;k++)for(b=1;b<=10;b++){
  m=order[k]; key=m SUBSEP b
  if(n[key]!=30){print "ERROR: missing bins for "m" "b > "/dev/stderr";exit 2}
  mean=sx[key]/n[key]; variance=(sq[key]-sx[key]*sx[key]/n[key])/(n[key]-1)
  printf "%s\t%d\t%.4f\t%.4f\t%.4f\t%.4f\n",m,b*10000,mean,sqrt(variance>0?variance:0),se[key]/n[key],st[key]/n[key]
 }
}' "$OUT/per_run_bins.tsv" > "$OUT/trajectory.tsv"

cat > "$OUT/plot_acceptance.gnuplot" <<'GNUPLOT'
set terminal pngcairo size 1300,800 enhanced font 'Sans,12'
set datafile separator '\t'
set key outside right
set grid
set xlabel 'Fitness evaluations (end of 10,000-evaluation bin)'
set ylabel 'Mean accepted crossover improvements per 10,000 fitness evaluations'
set output 'acceptance_trajectory.png'
plot for [i=0:5] 'trajectory.tsv' using (strcol(1) eq word('standard standardElite standardBlock targeted targetedFull targetedWorst',i+1) ? $2 : 1/0):3 with linespoints lw 2 pt (i+1) title word('standard standardElite standardBlock targeted targetedFull targetedWorst',i+1)

set output 'acceptance_per_evaluation.png'
set ylabel 'Mean accepted improvements per 1,000 crossover evaluations in bin'
plot for [i=0:5] 'trajectory.tsv' using (strcol(1) eq word('standard standardElite standardBlock targeted targetedFull targetedWorst',i+1) ? $2 : 1/0):($5>0 ? 1000*$3/$5 : 1/0) with linespoints lw 2 pt (i+1) title word('standard standardElite standardBlock targeted targetedFull targetedWorst',i+1)
GNUPLOT

if command -v gnuplot >/dev/null 2>&1; then
  (cd "$OUT" && gnuplot plot_acceptance.gnuplot)
  echo "Charts: $OUT/acceptance_trajectory.png and acceptance_per_evaluation.png"
else
  echo "gnuplot not installed; run: cd '$OUT' && gnuplot plot_acceptance.gnuplot"
fi
printf '\nSummary:\n'
column -t -s $'\t' "$OUT/summary.tsv" 2>/dev/null || cat "$OUT/summary.tsv"
echo "\nOutput directory: $OUT"
