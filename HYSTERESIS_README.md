Adaptive hysteresis (2026-11-05)
================================
New option: --adaptive-hysteresis 0.02 (default).
The currently active weight triple is re-evaluated on the current population and
validation set. A different triple is adopted only when its validation score is
at least 0.02 higher than the incumbent's score. The initial selection at
generation 0 is not subject to hysteresis. No normalization, stratification or
SMOTE has been added. Set --adaptive-hysteresis 0 to disable the threshold.
The ADAPTIVE_HYSTERESIS log prints incumbent_score, candidate_score, kept.
This is an experimental change; it does not guarantee improved test F1.
