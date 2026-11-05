# adaptive_mixed_minority_f1

Adds a third adaptive validation metric alongside `adaptive_mixed` (macro F1) and `adaptive_mixed_gmean` (G-mean).

The smallest class is identified by counts in the internal training partition. Validation uses its F1 = 2TP/(2TP+FP+FN), with zero for a zero denominator. The original unstratified random 80/20 split, 66 weight combinations, and existing mixed training fitness remain unchanged. The external test fold is not used to select weights.

Run:

```bash
./runmodel.sh --dataset urban_2014_1 --fitness adaptive_mixed_minority_f1 --generations 2000 --adaptive-interval 20 --adaptive-validation 0.2 --runs 1 --enable-smote no --data-dir /home/itsoulos/DORA/PERIF13XOROI17/
```
