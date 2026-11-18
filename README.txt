QGenClass equal-budget mutation patch

Copy GE/population.cc and GE/population.h to your project GE/ directory,
and runmodel.sh to the project root. Back up originals first.

New methods: active_equal, targeted_equal.
Original standard, active, targeted are retained.

Equal methods draw Binomial(genome_size, mutation_rate) mutation attempts
per chromosome and choose active/semantic positions WITH replacement.
Consequently they match Standard in expected number of mutation attempts,
not necessarily in distinct changed positions or phenotype impact.

With --local none, periodic local search, stagnation burst, and local
crossover are skipped. Standard mutation behavior is otherwise preserved.

Build: make -j$(nproc)
Test: ./runmodel.sh --dataset ionosphere --folds 10 --runs 1 --generations 100 --chromosomes 200 --mutation-method active_equal --local none

CAUTION: inspect output and compare with baseline before launching long runs.
Existing scripts create new summary files; they do NOT resume interrupted jobs.
