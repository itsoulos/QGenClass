# QGenClass: validation-based fitness selection

This script works with the **existing QGenClass executable and `runmodel.sh`**; no C++ changes are necessary. It expects the native `data` format: feature dimension on line 1, number of examples on line 2, then one feature vector plus class label per line.

## Run

Copy `select_fitness.py` into your QGenClass project directory, then:

```bash
python3 select_fitness.py \
  --project . \
  --dataset urban_2014_1 \
  --data-dir /home/itsoulos/DORA/PERIF13XOROI17 \
  --output results/urban_validation \
  --splits 3 --seeds-per-split 1 \
  --generations 500 --chromosomes 500 \
  --final-runs 10 --criterion minority_f2
```

Candidates: `mean`, `minority_f1`, `minority_fbeta` (beta=2 and 3), `hybrid_sampling` (lambda=.90, ratio=3). Validation is stratified and repeated using the same splits and seeds for all candidates. Only training data are used for selection. Once selected, the chosen fitness is run on the complete original training set and evaluated on the original test set for 10 seeds.

Output: `validation_runs.csv`, `validation_ranking.csv`, `selected.txt`, `final_test_runs.csv`, and per-run logs. Add `--skip-final` to tune without touching the test set. Use `--criterion balanced_accuracy` if that is your primary objective instead of minority F2.

**Note:** With very few minority observations, the validation score can be noisy. The final test metrics are for evaluation only: do not use them to reselect fitness or parameters. Repeated seeds on a fixed test set quantify training randomness, not generalization uncertainty across new test sets. Do not enable SMOTE in this script. It assumes a binary classification dataset with labels in the last column and the native `.train`/`.test` format.
