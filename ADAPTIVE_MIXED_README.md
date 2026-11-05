# Adaptive MIXED (experimental)

Uses a stratified 20% validation subset taken ONLY from the supplied training fold.
Each 20 generations it evaluates the incumbent and six neighboring weight triples.
For each triple, it selects the best chromosome by training MIXED loss and scores that chromosome on held-out validation Macro F1. The weights are updated only if the best validation score improves the previous record. All chromosome fitnesses are refreshed after the update.

Example:

```bash
./runmodel.sh --dataset texture_1 --fitness adaptive_mixed --generations 500 --adaptive-interval 20 --adaptive-step 0.05 --adaptive-validation 0.2
```

For runmodel_30.sh, set `ADAPTIVE_INTERVAL`, `ADAPTIVE_STEP`, `ADAPTIVE_VALIDATION` environment variables.

Important: this is an experimental inner holdout method, not a nested-CV estimate. The validation subset is not used to train rules, but repeated tuning against it can still overfit. The external test fold must remain untouched. For feature-construction mode, feature construction currently precedes the split, so validation leakage via feature construction is possible; use standard mode for clean adaptive experiments.
