# QGenClass Repeated Experiment Runner

## Overview

`runmodel_30_updated.sh` runs repeated QGenClass classification
experiments with different random seeds and summarizes their results.

The current script supports:

-   repeated experiments (30 runs by default);
-   a different random seed for every run;
-   selectable local-search methods;
-   selectable fitness methods;
-   standard and targeted local-crossover methods;
-   targeted local-search parameters;
-   targeted crossover parameters;
-   separate log files for every run;
-   automatic extraction of classification metrics;
-   execution-time measurement;
-   final average, sample standard deviation, minimum, and maximum
    statistics.

------------------------------------------------------------------------

## Requirements

The script assumes that the QGenClass executable is available as:

``` bash
./QGenClass
```

The default dataset directory is:

``` text
$HOME/Desktop/ERGASIES/FeatureConstruction2/datasets/tenfolding
```

A dataset named `ionosphere_1`, for example, must contain:

``` text
ionosphere_1.train
ionosphere_1.test
```

By default both files use QGenClass `data` format.

Make the script executable with:

``` bash
chmod +x runmodel_30_updated.sh
```

------------------------------------------------------------------------

# Command-Line Syntax

``` bash
./runmodel_30_updated.sh DATASET [REPETITIONS] [BASE_SEED] [LOCAL_METHOD] [FITNESS_METHOD] [CROSS_METHOD]
```

Only `DATASET` is required.

The defaults are:

``` text
REPETITIONS    = 30
BASE_SEED      = 1
LOCAL_METHOD   = targeted
FITNESS_METHOD = class
CROSS_METHOD   = targeted
```

Therefore:

``` bash
./runmodel_30_updated.sh ionosphere_1
```

is equivalent to:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targeted class targeted
```

------------------------------------------------------------------------

# Command-Line Parameters

## DATASET

Dataset name without `.train` or `.test`.

Example:

``` bash
./runmodel_30_updated.sh ionosphere_1
```

uses:

``` text
.../ionosphere_1.train
.../ionosphere_1.test
```

------------------------------------------------------------------------

## REPETITIONS

Number of independent QGenClass runs.

Default:

``` text
30
```

Example:

``` bash
./runmodel_30_updated.sh ionosphere_1 50
```

performs 50 runs.

The value must be a positive integer.

------------------------------------------------------------------------

## BASE_SEED

Random seed for the first run.

Default:

``` text
1
```

The script increments it by one after each run.

For:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 100 targeted class targeted
```

the seeds are:

``` text
100, 101, 102, ..., 129
```

This gives different but reproducible random sequences for the repeated
experiments.

------------------------------------------------------------------------

# Local Search

## LOCAL_METHOD

The fourth command-line argument selects the local-search operator.

Supported values are:

  -----------------------------------------------------------------------
  Method                              Purpose
  ----------------------------------- -----------------------------------
  `none`                              Disable local search

  `crossover`                         Crossover-based local improvement

  `mutate`                            Mutation-based local search

  `siman`                             Simulated annealing

  `hill`                              Hill climbing

  `de`                                Differential-evolution-based local
                                      search

  `gd`                                Gradient-descent-style local search

  `adam`                              Adam-style local optimization

  `mutateWorst`                       Mutation concentrated on the
                                      worst-performing class

  `targeted`                          Targeted semantic mutation

  `targetedWorst`                     Targeted semantic mutation focused
                                      on the worst class

  `targetedBest`                      Best-improvement targeted local
                                      search

  `constants`                         Local optimization restricted to
                                      numerical constants
  -----------------------------------------------------------------------

Default:

``` text
targeted
```

Example:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targetedWorst class targeted
```

### Numerical-constant optimization

To optimize only numerical constants:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 constants class targeted
```

This selects `constants` as the local-search method while keeping
targeted crossover enabled.

------------------------------------------------------------------------

# Fitness Method

## FITNESS_METHOD

Supported values are:

``` text
class
average
squared
mixed
mean
macroF1
weightedF1
```

Default:

``` text
class
```

Example:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targeted macroF1 targeted
```

------------------------------------------------------------------------

# Local Crossover

## CROSS_METHOD

The sixth command-line argument controls the local crossover strategy.

Supported values are:

  -----------------------------------------------------------------------
  Method                              Description
  ----------------------------------- -----------------------------------
  `standard`                          Original QGenClass local crossover

  `targeted`                          Targeted semantic crossover

  `targetedWorst`                     Targeted semantic crossover focused
                                      on the worst-performing class
  -----------------------------------------------------------------------

Default:

``` text
targeted
```

### Standard crossover

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targeted class standard
```

### Targeted crossover

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targeted class targeted
```

### Worst-class targeted crossover

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targetedWorst class targetedWorst
```

The last example uses both worst-class targeted local search and
worst-class targeted crossover.

------------------------------------------------------------------------

# Genetic Algorithm Parameters

The following parameters are configured through environment variables.

## POP_COUNT

Number of chromosomes in the population.

Default:

``` text
500
```

QGenClass option:

``` text
--pop_count
```

Example:

``` bash
POP_COUNT=1000 ./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

## POP_SIZE

Chromosome size.

Default:

``` text
200
```

QGenClass option:

``` text
--pop_size
```

Example:

``` bash
POP_SIZE=400 ./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

## POP_GENS

Maximum number of generations.

Default:

``` text
2000
```

QGenClass option:

``` text
--pop_gens
```

Example:

``` bash
POP_GENS=5000 ./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

## POP_SRATE

Selection rate.

Default:

``` text
0.1
```

QGenClass option:

``` text
--pop_srate
```

------------------------------------------------------------------------

## POP_MRATE

Mutation rate.

Default:

``` text
0.05
```

QGenClass option:

``` text
--pop_mrate
```

Example:

``` bash
POP_MRATE=0.02 ./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

# Local-Search Parameters

## POP_LOCALITEMS

Number of population members participating in local search.

Default:

``` text
20
```

QGenClass option:

``` text
--pop_localitems
```

------------------------------------------------------------------------

## POP_LOCALGENS

Number of generations between local-search applications.

Default:

``` text
50
```

QGenClass option:

``` text
--pop_localgens
```

Example:

``` bash
POP_LOCALGENS=20 ./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

# Targeted Local-Search Parameters

## TARGETED_ITERATIONS

Number of targeted local-search iterations.

Default:

``` text
50
```

QGenClass option:

``` text
--targeted_iterations
```

Example:

``` bash
TARGETED_ITERATIONS=100 \
./runmodel_30_updated.sh ionosphere_1 30 1 targeted class targeted
```

------------------------------------------------------------------------

## TARGETED_STAGNATION_LIMIT

Stagnation threshold used by the targeted-search mechanism.

Default:

``` text
20
```

QGenClass option:

``` text
--targeted_stagnationlimit
```

Example:

``` bash
TARGETED_STAGNATION_LIMIT=10 \
./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

## TARGETED_BURST_ITERATIONS

Number of iterations used for a targeted search burst.

Default:

``` text
200
```

QGenClass option:

``` text
--targeted_burstiterations
```

Example:

``` bash
TARGETED_BURST_ITERATIONS=500 \
./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

# Targeted Crossover Parameters

## POP_CROSSITEMS

Number of chromosomes participating in local crossover.

Default:

``` text
10
```

QGenClass option:

``` text
--pop_crossitems
```

Example:

``` bash
POP_CROSSITEMS=20 \
./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

## TARGETED_CROSS_ITERATIONS

Number of targeted crossover attempts.

Default:

``` text
20
```

QGenClass option:

``` text
--targeted_crossiterations
```

Example:

``` bash
TARGETED_CROSS_ITERATIONS=50 \
./runmodel_30_updated.sh ionosphere_1 30 1 targeted class targeted
```

Increasing this value gives targeted crossover more opportunities to
find an improving candidate, but also increases computational cost.

------------------------------------------------------------------------

## TARGETED_CROSS_ELITE_FRACTION

Fraction of the population used as the elite donor pool by targeted
crossover.

Default:

``` text
0.2
```

QGenClass option:

``` text
--targeted_crosselitefraction
```

With:

``` text
0.2
```

the targeted crossover uses the top 20% of the population as its donor
pool.

Example:

``` bash
TARGETED_CROSS_ELITE_FRACTION=0.10 \
./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

## TARGETED_CROSS_MAX_BLOCK

Maximum number of codons copied by a targeted crossover operation.

Default:

``` text
16
```

QGenClass option:

``` text
--targeted_crossmaxblock
```

Example:

``` bash
TARGETED_CROSS_MAX_BLOCK=8 \
./runmodel_30_updated.sh ionosphere_1
```

A smaller value makes crossover more local, while a larger value permits
larger genotype modifications.

------------------------------------------------------------------------

# Fitness Weight Parameters

The script defines:

``` text
POP_CLASSPERCENT   = 0.50
POP_AVERAGEPERCENT = 0.50
POP_SQUAREDPERCENT = 0.00
```

These are passed as:

``` text
--pop_classpercent
--pop_averagepercent
--pop_squaredpercent
```

They are especially relevant when using a fitness mode that combines
multiple components.

Example:

``` bash
POP_CLASSPERCENT=0.7 \
POP_AVERAGEPERCENT=0.3 \
POP_SQUAREDPERCENT=0.0 \
./runmodel_30_updated.sh ionosphere_1 30 1 targeted mixed targeted
```

------------------------------------------------------------------------

# Dataset Parameters

## TRAIN_FORMAT

Default:

``` text
data
```

Passed as:

``` text
--train_format
```

## TEST_FORMAT

Default:

``` text
data
```

Passed as:

``` text
--test_format
```

## ENABLE_SMOTE

Default:

``` text
no
```

Passed as:

``` text
--enable_smote
```

Example:

``` bash
ENABLE_SMOTE=yes ./runmodel_30_updated.sh ionosphere_1
```

## SMOTE_K

Default:

``` text
5
```

Passed as:

``` text
--smote_k
```

Example:

``` bash
ENABLE_SMOTE=yes \
SMOTE_K=7 \
./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

# Paths

## DATAPATH

Default dataset directory:

``` text
$HOME/Desktop/ERGASIES/FeatureConstruction2/datasets/tenfolding
```

Override it with:

``` bash
DATAPATH=/home/user/datasets \
./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

## PROGRAM

Default executable:

``` text
./QGenClass
```

Override it with:

``` bash
PROGRAM=/home/user/QGenClass/QGenClass \
./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

## RESULTSDIR

Default:

``` text
results
```

Example:

``` bash
RESULTSDIR=experiment_results \
./runmodel_30_updated.sh ionosphere_1
```

------------------------------------------------------------------------

# Output Files

For a configuration such as:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targeted class targeted
```

the experiment tag is:

``` text
ionosphere_1_targeted_class_targeted
```

The script creates:

``` text
results/results_ionosphere_1_targeted_class_targeted.txt
results/summary_ionosphere_1_targeted_class_targeted.txt
```

and individual run logs such as:

``` text
results/run_ionosphere_1_targeted_class_targeted_seed1.log
results/run_ionosphere_1_targeted_class_targeted_seed2.log
...
results/run_ionosphere_1_targeted_class_targeted_seed30.log
```

------------------------------------------------------------------------

# Progress Output

During execution the script displays the current run:

``` text
RUN 1 / 30
SEED = 1
```

and selected QGenClass progress information.

It recognizes output associated with:

``` text
TARGETED
TARGETED_WORST
TARGETED_BEST
CONSTANTS
TARGETED_CROSS
TARGETED_CROSS_WORST
```

The final classification metric line is also displayed for each run.

------------------------------------------------------------------------

# Metrics

The script extracts the final QGenClass line of the form:

``` text
CLASS_ERROR: value PRECISION: value RECALL: value F1SCORE: value
```

It records:

-   Class Error
-   Accuracy
-   Precision
-   Recall
-   F1 Score
-   Execution Time

Accuracy is calculated from the percentage class error:

\[ Accuracy = 1 - `\frac{ClassError}{100}`{=tex} \]

For example, if:

``` text
CLASS_ERROR = 11.4285714286
```

then:

``` text
Accuracy = 0.8857142857
```

------------------------------------------------------------------------

# Detailed Results File

The results file contains:

``` text
Run
Seed
ClassError
Accuracy
Precision
Recall
F1Score
TimeSeconds
LogFile
```

Example structure:

``` text
# Run Seed ClassError Accuracy Precision Recall F1Score TimeSeconds LogFile
1 1 11.4285714286 0.8857142857 0.9130434783 0.8750000000 0.8936170213 72 results/run_...
2 2 8.5714285714 0.9142857143 0.9200000000 0.9000000000 0.9098901099 69 results/run_...
```

------------------------------------------------------------------------

# Final Statistics

After all runs finish, the script reports:

``` text
Average
Std.Dev.
Minimum
Maximum
```

for:

``` text
Class Error (%)
Accuracy
Precision
Recall
F1 Score
Time (sec)
```

For a metric (x) measured over (N) runs, the mean is:

\[ `\bar`{=tex}{x} = `\frac{1}{N}`{=tex}`\sum`{=tex}\_{i=1}\^{N}x_i \]

The script uses the sample standard deviation:

\[ s = `\sqrt{
\frac{
\sum_{i=1}^{N}(x_i-\bar{x})^2
}{
N-1
}
}`{=tex} \]

when more than one run is available.

------------------------------------------------------------------------

# Useful Experiment Examples

## Baseline: no local search, standard crossover

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 none class standard
```

## Targeted local search with standard crossover

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targeted class standard
```

## Targeted local search and targeted crossover

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targeted class targeted
```

## Worst-class targeted search and crossover

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targetedWorst class targetedWorst
```

## Best-improvement local search with targeted crossover

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targetedBest class targeted
```

## Constant optimization with targeted crossover

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 constants class targeted
```

## Macro-F1 optimization

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targeted macroF1 targeted
```

------------------------------------------------------------------------

# Comparing Crossover Methods

A controlled comparison can be made by keeping the same dataset, seeds,
local-search method, and fitness method while changing only
`CROSS_METHOD`:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targeted class standard

./runmodel_30_updated.sh ionosphere_1 30 1 targeted class targeted

./runmodel_30_updated.sh ionosphere_1 30 1 targeted class targetedWorst
```

Because all three experiments use the same seed sequence, their
aggregate results are easier to compare.

------------------------------------------------------------------------

# Comparing Local-Search Methods

Similarly:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 none class targeted

./runmodel_30_updated.sh ionosphere_1 30 1 mutateWorst class targeted

./runmodel_30_updated.sh ionosphere_1 30 1 targeted class targeted

./runmodel_30_updated.sh ionosphere_1 30 1 targetedWorst class targeted

./runmodel_30_updated.sh ionosphere_1 30 1 targetedBest class targeted

./runmodel_30_updated.sh ionosphere_1 30 1 constants class targeted
```

------------------------------------------------------------------------

# Changing Several Parameters at Once

Example:

``` bash
POP_COUNT=1000 \
POP_SIZE=300 \
POP_GENS=5000 \
POP_MRATE=0.02 \
POP_LOCALITEMS=10 \
POP_LOCALGENS=25 \
TARGETED_ITERATIONS=100 \
TARGETED_STAGNATION_LIMIT=10 \
TARGETED_BURST_ITERATIONS=500 \
POP_CROSSITEMS=20 \
TARGETED_CROSS_ITERATIONS=50 \
TARGETED_CROSS_ELITE_FRACTION=0.20 \
TARGETED_CROSS_MAX_BLOCK=8 \
./runmodel_30_updated.sh ionosphere_1 30 100 targetedBest macroF1 targeted
```

This allows parameter studies without editing the script itself.

------------------------------------------------------------------------

# Recommended Workflow

For a quick configuration check:

``` bash
POP_GENS=100 \
./runmodel_30_updated.sh ionosphere_1 2 1 targeted class targeted
```

After confirming that everything works, run the full experiment:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targeted class targeted
```

When comparing algorithms, keep the same:

-   dataset;
-   number of repetitions;
-   base seed;
-   population size;
-   chromosome size;
-   number of generations;
-   mutation and selection rates.

Change only the algorithmic component under investigation.

For example, to study crossover, change only `CROSS_METHOD`.

------------------------------------------------------------------------

# Error Checking

Before execution, the script checks:

-   that `REPETITIONS` is a positive integer;
-   that `BASE_SEED` is a non-negative integer;
-   that the selected local-search method is recognized;
-   that the selected fitness method is recognized;
-   that the selected crossover method is recognized;
-   that the QGenClass executable exists and is executable;
-   that the training file exists;
-   that the test file exists.

The experiment stops if QGenClass returns a non-zero exit status.

It also stops if the final `CLASS_ERROR` metric line cannot be found or
parsed.

------------------------------------------------------------------------

# Quick Reference

Default 30-run experiment:

``` bash
./runmodel_30_updated.sh ionosphere_1
```

Targeted search + targeted crossover:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targeted class targeted
```

Worst-class targeted search + worst-class targeted crossover:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 targetedWorst class targetedWorst
```

Constant optimization:

``` bash
./runmodel_30_updated.sh ionosphere_1 30 1 constants class targeted
```

Larger experiment:

``` bash
POP_COUNT=1000 \
POP_GENS=5000 \
TARGETED_ITERATIONS=100 \
TARGETED_CROSS_ITERATIONS=50 \
./runmodel_30_updated.sh ionosphere_1 30 1 targetedBest macroF1 targeted
```

------------------------------------------------------------------------

## Notes

This document describes the current `runmodel_30_updated.sh` interface
and the QGenClass options passed by that script.

The aggregate metrics depend on the final QGenClass output line:

``` text
CLASS_ERROR: ... PRECISION: ... RECALL: ... F1SCORE: ...
```

If the QGenClass reporting format changes, the metric-parsing section of
the script may also need to be updated.
