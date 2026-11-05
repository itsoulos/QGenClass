# adaptive_mixed_gmean

Adds `adaptive_mixed_gmean` alongside `adaptive_mixed` and `mixed`.

- 66 simplex weights (step 0.1) and updates every `adaptive_interval` generations.
- `adaptive_mixed_gmean` selects weights by validation G-mean; `adaptive_mixed` continues to use validation Macro F1.
- Internal validation uses a **random, unstratified** holdout from the outer training fold. It does not rebalance or normalize data.
- Missing validation classes or zero class recall produce G-mean zero; this may happen with very rare classes and a small random validation set. The method does not guarantee minority-class coverage.
- To avoid data modifications, run with `--enable-smote no` (the default) and without feature-construction mode. Existing optional SMOTE behavior is unchanged and remains user-controlled.
- Log fields `validation_score` and `metric` identify the selection criterion.
- External test fold is not used to choose weights.

Build requires Qt qmake and C++ compiler; not compiled in this environment (qmake unavailable).
