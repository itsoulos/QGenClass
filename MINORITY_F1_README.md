# Direct minority F1 fitness

Use `--fitness minority_f1`. This minimizes `100 * (1 - minority_F1)`
computed on the training set. The minority class is the least frequent
nonempty class in the training set. No resampling or normalization.
The pre-existing adaptive modes remain available.

Example:
```bash
./runmodel.sh --dataset urban_2014_1 --fitness minority_f1 --generations 2000 --runs 1 --enable-smote no --data-dir /home/itsoulos/DORA/PERIF13XOROI17/
```

This experiment uses the existing external test split; do not tune
parameters on the test results.
