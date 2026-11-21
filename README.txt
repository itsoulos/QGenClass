QGenClass guided mutation experimental patch

Back up original files before extracting. Run make -j$(nproc).
Methods: gradient, adam, semantic_adam.
Example: ./runmodel.sh --dataset ionosphere --folds 10 --runs 1 --generations 100 --chromosomes 100 --mutation-method semantic_adam --guided-steps 2 --guided-codons 4 --guided-budget 16 --guided-alpha 1 --guided-fraction 0.05 --local none

These are bounded integer finite-difference heuristics, not analytic gradient descent.
For guided_fraction of non-elite chromosomes, apply guided optimizer; others receive ordinary standard mutation.
The evaluation budget is per guided chromosome (includes initial fitness and final candidate evaluations).
The selected codons are re-sampled at each step; semantic_adam refreshes CodonTrace each step.
Use the same crossover, seeds, folds and evaluation budgets for meaningful comparisons.
WARNING: Not compiled against full QGenClass project in this environment. Run a short smoke test before long jobs.
