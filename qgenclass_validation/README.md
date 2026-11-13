# QGenClass validation selection (including constrained_recall)

Requires the QGenClass build with `constrained_recall` and its `runmodel.sh`, plus Python 3 (standard library only). Copy `select_fitness.py` to your QGenClass project directory.

Run from the QGenClass directory:

```bash
python3 select_fitness.py \
  --project . \
  --dataset urban_2014_1 \
  --data-dir /home/itsoulos/DORA/PERIF13XOROI17 \
  --output results/urban_validation_fpr \
  --splits 5 --seeds-per-split 3 \
  --generations 500 --chromosomes 500 \
  --criterion recall_at_fpr --fpr-limit 0.10 \
  --final-runs 10
```

The program evaluates the original five candidates plus `constrained_recall` with penalties 5, 10 and 20, on **identical** stratified training/validation splits and seeds. It never samples from the original test set during model selection. `recall_at_fpr` chooses the highest *mean* minority recall among candidates whose FPR is at most the limit **on every validation run**. If none qualifies, it selects the candidate with the lowest worst-case validation FPR (then highest recall). This is deliberately conservative, and does not guarantee a test-set FPR ceiling. To select on minority F2 instead, pass `--criterion minority_f2`.

Outputs: `validation_runs.csv`, `validation_ranking.csv`, `selected.txt`, `final_test_runs.csv`, logs under `validation/` and `final/`. Use `--skip-final` for tuning only. The script requires binary data files named `<dataset>.train` and `<dataset>.test`, formatted as dimension, pattern count, and data rows with the class label last. **The candidate FPR limits inside the fitness are fixed at 0.10**; keep `--fpr-limit 0.10` for consistent selection, or edit the candidate definitions if changing that target.

**Important:** The previous repeated inspections of the same test set have already informed experimental choices. For publishable unbiased assessment, evaluate the finalized procedure on a fresh untouched holdout or outer cross-validation folds. Small minority counts cause high variance; report per-class precision/recall and confusion matrices, not accuracy alone.
