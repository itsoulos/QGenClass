# QGenClass constrained_recall patch

Based on QGenClass.tar(20261113-163842).gz. Replace the four files preserving their paths.

New fitness: `constrained_recall`, minimized on training only:

`100 * [(1 - minority_recall) + fpr_penalty * max(0, majority_FPR - fpr_limit)]`

- `--fpr-limit` (default 0.10), range [0,1]
- `--fpr-penalty` (default 5.0), range [0,1000]
- Binary classification only; returns a large penalty otherwise.
- This is a **soft** constraint: it does not guarantee test FPR <= limit.
- Select penalty and limit using validation; never tune them on test data.

Example:

```bash
./runmodel.sh --dataset urban_2014_1 --data-dir /home/itsoulos/DORA/PERIF13XOROI17 \
  --fitness constrained_recall --fpr-limit 0.10 --fpr-penalty 5 \
  --generations 500 --chromosomes 500 --runs 10 --seed 1 --smote no \
  --results-dir results/urban_constrained_010_p5
```

The existing `select_fitness.py` does not automatically include this new candidate: update its candidate configuration before attempting automatic selection.
