# macroPR fitness

Minimize `100 * (1 - (alpha * macro_recall + (1-alpha) * macro_precision))`.
All macro metrics use the training class universe; zero denominators count as zero.

Build: `make -j$(nproc)`

Examples:

```bash
./runmodel.sh --dataset ionosphere_1 --fitness macroPR --macropr-alpha 0.5 --runs 10
./runmodel.sh --dataset ionosphere_1 --fitness macroPR --macropr-alpha 0.7 --runs 10
POP_MACROPR_ALPHA=0.6 ./runmodel_30.sh ionosphere_1 30 1 targeted macroPR targeted
```

The objective is a weighted arithmetic combination of macro precision and macro recall, not macro F1. It does not guarantee a better trade-off on test data.
