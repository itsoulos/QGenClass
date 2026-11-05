# Adaptive MIXED: full simplex grid (experimental)

At generation 0 and every `adaptive_interval` generations (default 20),
search all 66 nonnegative weight triples summing to 1 in increments of 0.1.
The three component training losses are computed only once per chromosome
per adaptation step. For each triple, select the lowest-loss chromosome from
the **existing** population and evaluate its Macro F1 on the inner stratified
validation subset (cached per chromosome). The highest-validation-score
triple is selected. Logs include 66 `ADAPTIVE_CANDIDATE` entries and one
`ADAPTIVE_MIXED` summary per adaptation.

Example:

```bash
./runmodel.sh --dataset texture_1 --fitness adaptive_mixed --generations 500 --adaptive-interval 20 --adaptive-validation 0.2 --runs 1
```

`--adaptive-step` is retained for backwards-compatible CLI parsing but is
**not used** by grid search. The original `mixed` fitness is unchanged.

Caveats: repeated tuning on one validation split can overfit; this is not
nested cross-validation. The external test set is never used to choose
weights. Use standard mode: feature-construction mode currently builds
features before the validation split, potentially leaking validation data.

The first grid evaluation may pick a boundary weight triple. If many
triples select the same genome, weight changes alone cannot create a
validation improvement; the logs expose this behavior.
