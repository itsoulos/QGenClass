# QGenClass Repeated Experiment Runner

## Overview

`runmodel_30.sh` is a Bash script for running repeated QGenClass
classification experiments with different random seeds.

The script is designed to make experimental evaluation easier by:

-   running the same dataset multiple times;
-   automatically changing the random seed between runs;
-   supporting the available QGenClass local-search methods;
-   storing a separate log for every run;
-   extracting the final classification metrics from QGenClass output;
-   measuring the execution time of each run;
-   calculating average, standard deviation, minimum, and maximum values
    across all runs.

By default, the script performs **30 independent runs**.

------------------------------------------------------------------------

## Requirements

The script assumes that:

1.  QGenClass has already been compiled.
2.  The executable is available as:

``` bash
./QGenClass
```

3.  The datasets are stored in:

``` text
~/Desktop/ERGASIES/FeatureConstruction2/datasets/tenfolding
```

4.  Every dataset consists of two files:

``` text
dataset.train
dataset.test
```

For example:

``` text
ionosphere_1.train
ionosphere_1.test
```

5.  The training and test files use the QGenClass `data` format.

------------------------------------------------------------------------

## Making the Script Executable

After copying or downloading the script, make it executable:

``` bash
chmod +x runmodel_30.sh
```

------------------------------------------------------------------------

## Basic Syntax

The command-line syntax is:

``` bash
./runmodel_30.sh DATASET [REPETITIONS] [BASE_SEED] [LOCAL_METHOD] [FITNESS_METHOD]
```

Only `DATASET` is required.

The remaining parameters are optional.

------------------------------------------------------------------------

## Command-Line Parameters

### `DATASET`

The dataset name without the `.train` or `.test` suffix.

For example, if the files are:

``` text
ionosphere_1.train
ionosphere_1.test
```

use:

``` bash
./runmodel_30.sh ionosphere_1
```

The script automatically constructs the complete training and testing
filenames.

------------------------------------------------------------------------

### `REPETITIONS`

Number of independent experiments.

Default:

``` text
30
```

Example:

``` bash
./runmodel_30.sh ionosphere_1 50
```

This performs 50 independent experiments.

The value must be a positive integer.

------------------------------------------------------------------------

### `BASE_SEED`

Random seed used for the first experiment.

Default:

``` text
1
```

The seed is increased by one after every run.

For example:

``` bash
./runmodel_30.sh ionosphere_1 30 100
```

uses:

``` text
Run 1  -> seed 100
Run 2  -> seed 101
Run 3  -> seed 102
...
Run 30 -> seed 129
```

This makes the experiments reproducible while ensuring that every run
starts from a different random state.

------------------------------------------------------------------------

### `LOCAL_METHOD`

Selects the QGenClass local-search method.

Default:

``` text
targeted
```

Supported values in the script are:

  -----------------------------------------------------------------------
  Method                              Description
  ----------------------------------- -----------------------------------
  `none`                              No local-search operator

  `crossover`                         Local improvement based on
                                      crossover

  `mutate`                            Mutation-based local search

  `siman`                             Simulated annealing

  `hill`                              Hill-climbing local search

  `de`                                Differential-evolution-based local
                                      search

  `gd`                                Gradient-descent-style local search

  `adam`                              Adam-style local optimization

  `mutateWorst`                       Mutation concentrated on the class
                                      with the largest error

  `targeted`                          Semantic targeted mutation of
                                      active codons

  `targetedWorst`                     Targeted mutation concentrated on
                                      the worst-performing class

  `targetedBest`                      Best-improvement targeted search
                                      over active semantic mutations

  `constants`                         Local optimization restricted to
                                      numerical constants
  -----------------------------------------------------------------------

Example:

``` bash
./runmodel_30.sh ionosphere_1 30 1 targetedWorst class
```

------------------------------------------------------------------------

### `FITNESS_METHOD`

Selects the QGenClass fitness method.

Default:

``` text
class
```

Supported values in the script are:

``` text
class
average
squared
mixed
mean
macroF1
weightedF1
```

Example:

``` bash
./runmodel_30.sh ionosphere_1 30 1 targeted macroF1
```

------------------------------------------------------------------------

## Default Execution

Running:

``` bash
./runmodel_30.sh ionosphere_1
```

is equivalent to:

``` bash
./runmodel_30.sh ionosphere_1 30 1 targeted class
```

Therefore the default experiment uses:

``` text
Dataset       : ionosphere_1
Repetitions   : 30
Base seed     : 1
Local method  : targeted
Fitness       : class
```

------------------------------------------------------------------------

## Examples

### 30 runs using targeted search

``` bash
./runmodel_30.sh ionosphere_1 30 1 targeted class
```

### 30 runs using targetedWorst

``` bash
./runmodel_30.sh ionosphere_1 30 1 targetedWorst class
```

### 30 runs using targetedBest

``` bash
./runmodel_30.sh ionosphere_1 30 1 targetedBest class
```

### Optimize only numerical constants

``` bash
./runmodel_30.sh ionosphere_1 30 1 constants class
```

### Use Macro-F1 as the fitness method

``` bash
./runmodel_30.sh ionosphere_1 30 1 targeted macroF1
```

### Start the seeds at 1000

``` bash
./runmodel_30.sh ionosphere_1 30 1000 targeted class
```

### Perform only five runs for a quick test

``` bash
./runmodel_30.sh ionosphere_1 5 1 targeted class
```

------------------------------------------------------------------------

# QGenClass Parameters

Most QGenClass parameters are controlled by environment variables in the
script.

Their default values are described below.

------------------------------------------------------------------------

## Population Size

Environment variable:

``` text
POP_COUNT
```

Default:

``` text
500
```

It is passed to QGenClass as:

``` text
--pop_count
```

Example:

``` bash
POP_COUNT=1000 ./runmodel_30.sh ionosphere_1 30 1 targeted class
```

------------------------------------------------------------------------

## Chromosome Size

Environment variable:

``` text
POP_SIZE
```

Default:

``` text
200
```

It is passed as:

``` text
--pop_size
```

Example:

``` bash
POP_SIZE=400 ./runmodel_30.sh ionosphere_1
```

------------------------------------------------------------------------

## Number of Generations

Environment variable:

``` text
POP_GENS
```

Default:

``` text
2000
```

It is passed as:

``` text
--pop_gens
```

Example:

``` bash
POP_GENS=5000 ./runmodel_30.sh ionosphere_1
```

------------------------------------------------------------------------

## Selection Rate

Environment variable:

``` text
POP_SRATE
```

Default:

``` text
0.1
```

It is passed as:

``` text
--pop_srate
```

------------------------------------------------------------------------

## Mutation Rate

Environment variable:

``` text
POP_MRATE
```

Default:

``` text
0.05
```

It is passed as:

``` text
--pop_mrate
```

Example:

``` bash
POP_MRATE=0.10 ./runmodel_30.sh ionosphere_1
```

------------------------------------------------------------------------

# Local Search Parameters

## Number of Local-Search Individuals

Environment variable:

``` text
POP_LOCALITEMS
```

Default:

``` text
20
```

It controls how many population members participate in local
improvement.

It is passed as:

``` text
--pop_localitems
```

Example:

``` bash
POP_LOCALITEMS=10 ./runmodel_30.sh ionosphere_1
```

------------------------------------------------------------------------

## Local-Search Interval

Environment variable:

``` text
POP_LOCALGENS
```

Default:

``` text
50
```

It is passed as:

``` text
--pop_localgens
```

Example:

``` bash
POP_LOCALGENS=20 ./runmodel_30.sh ionosphere_1
```

------------------------------------------------------------------------

## Targeted Iterations

Environment variable:

``` text
TARGETED_ITERATIONS
```

Default:

``` text
50
```

It is passed as:

``` text
--targeted_iterations
```

It controls the number of local-search iterations used by the targeted
methods.

Example:

``` bash
TARGETED_ITERATIONS=100 \
./runmodel_30.sh ionosphere_1 30 1 targeted class
```

------------------------------------------------------------------------

## Targeted Stagnation Limit

Environment variable:

``` text
TARGETED_STAGNATION_LIMIT
```

Default:

``` text
20
```

It is passed as:

``` text
--targeted_stagnationlimit
```

This parameter is used by the stagnation-detection mechanism. A targeted
search burst can be activated after the specified number of generations
without improvement.

Example:

``` bash
TARGETED_STAGNATION_LIMIT=10 \
./runmodel_30.sh ionosphere_1 30 1 targetedBest class
```

------------------------------------------------------------------------

## Targeted Burst Iterations

Environment variable:

``` text
TARGETED_BURST_ITERATIONS
```

Default:

``` text
200
```

It is passed as:

``` text
--targeted_burstiterations
```

It controls the strength of the targeted local-search burst used after
stagnation.

Example:

``` bash
TARGETED_BURST_ITERATIONS=500 \
./runmodel_30.sh ionosphere_1 30 1 targetedBest class
```

------------------------------------------------------------------------

# Fitness Weight Parameters

The script also defines:

``` text
POP_CLASSPERCENT
POP_AVERAGEPERCENT
POP_SQUAREDPERCENT
```

with defaults:

``` text
POP_CLASSPERCENT   = 0.50
POP_AVERAGEPERCENT = 0.50
POP_SQUAREDPERCENT = 0.00
```

They are passed to QGenClass as:

``` text
--pop_classpercent
--pop_averagepercent
--pop_squaredpercent
```

These parameters are particularly relevant when the selected QGenClass
fitness method uses a combination of fitness components.

Example:

``` bash
POP_CLASSPERCENT=0.7 \
POP_AVERAGEPERCENT=0.3 \
POP_SQUAREDPERCENT=0.0 \
./runmodel_30.sh ionosphere_1 30 1 targeted mixed
```

------------------------------------------------------------------------

# SMOTE Parameters

## Enable SMOTE

Environment variable:

``` text
ENABLE_SMOTE
```

Default:

``` text
no
```

It is passed as:

``` text
--enable_smote
```

Example:

``` bash
ENABLE_SMOTE=yes \
./runmodel_30.sh ionosphere_1
```

------------------------------------------------------------------------

## Number of SMOTE Neighbors

Environment variable:

``` text
SMOTE_K
```

Default:

``` text
5
```

It is passed as:

``` text
--smote_k
```

Example:

``` bash
ENABLE_SMOTE=yes \
SMOTE_K=7 \
./runmodel_30.sh ionosphere_1
```

------------------------------------------------------------------------

# Changing Multiple Parameters

Environment variables make it possible to configure an experiment
without editing the script.

For example:

``` bash
POP_COUNT=1000 \
POP_SIZE=300 \
POP_GENS=5000 \
POP_MRATE=0.02 \
POP_LOCALITEMS=10 \
POP_LOCALGENS=25 \
TARGETED_ITERATIONS=100 \
TARGETED_STAGNATION_LIMIT=15 \
TARGETED_BURST_ITERATIONS=400 \
./runmodel_30.sh ionosphere_1 30 100 targetedBest macroF1
```

This is useful for automated parameter studies because the original
script does not have to be modified for each experiment.

------------------------------------------------------------------------

# Output Directory

By default, results are stored in:

``` text
results/
```

The directory is created automatically if it does not already exist.

The output directory can be changed using:

``` text
RESULTSDIR
```

Example:

``` bash
RESULTSDIR=my_experiments \
./runmodel_30.sh ionosphere_1
```

------------------------------------------------------------------------

# Log Files

Every repetition has its own log file.

The filename contains:

-   dataset;
-   local-search method;
-   fitness method;
-   random seed.

For example:

``` text
results/run_ionosphere_1_targeted_class_seed1.log
results/run_ionosphere_1_targeted_class_seed2.log
results/run_ionosphere_1_targeted_class_seed3.log
```

This makes it possible to inspect the complete QGenClass output for
every individual experiment.

------------------------------------------------------------------------

# Detailed Results File

The script creates a results file such as:

``` text
results/results_ionosphere_1_targeted_class.txt
```

The columns are:

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

For example:

``` text
# Run Seed ClassError Accuracy Precision Recall F1Score TimeSeconds LogFile
1 1 11.4285714286 0.8857142857 0.9130434783 0.8750000000 0.8936170213 72 results/run_...
2 2 8.5714285714 0.9142857143 0.9200000000 0.9000000000 0.9098901099 69 results/run_...
```

------------------------------------------------------------------------

# Metrics Extracted from QGenClass

The script searches each run log for the final line having the form:

``` text
CLASS_ERROR: value PRECISION: value RECALL: value F1SCORE: value
```

For example:

``` text
CLASS_ERROR: 11.4285714286 PRECISION: 0.9130434783 RECALL: 0.8750000000 F1SCORE: 0.8936170213
```

The script extracts:

-   Class Error
-   Precision
-   Recall
-   F1 Score

Accuracy is then calculated from the percentage class error as:

``` text
Accuracy = 1 - ClassError / 100
```

For example:

``` text
ClassError = 11.4285714286%
```

gives:

``` text
Accuracy = 0.8857142857
```

------------------------------------------------------------------------

# Summary File

After all repetitions finish, the script creates a summary file such as:

``` text
results/summary_ionosphere_1_targeted_class.txt
```

For every metric, it reports:

``` text
Average
Standard deviation
Minimum
Maximum
```

The summary contains statistics for:

``` text
Class Error (%)
Accuracy
Precision
Recall
F1 Score
Execution Time
```

A typical summary has the form:

``` text
================================================================================
 FINAL STATISTICS
================================================================================

Metric                      Average       Std.Dev.        Minimum        Maximum
--------------------------------------------------------------------------------
Class Error (%)            ...
Accuracy                   ...
Precision                  ...
Recall                     ...
F1 Score                   ...
Time (sec)                 ...

Number of runs = 30
================================================================================
```

------------------------------------------------------------------------

# Standard Deviation

For (N) runs with metric values (x_1,x_2,`\ldots`{=tex},x_N), the script
first calculates the sample mean:

\[ `\bar`{=tex}{x}=`\frac{1}{N}`{=tex}`\sum`{=tex}\_{i=1}\^{N}x_i \]

and then the sample standard deviation:

\[ s= `\sqrt{
\frac{
\sum_{i=1}^{N}(x_i-\bar{x})^2
}{
N-1
}
}`{=tex} \]

The sample standard deviation is used when more than one run is
available.

------------------------------------------------------------------------

# Execution Time

The script measures both:

1.  the execution time of every individual run;
2.  the total execution time of the complete experiment.

A run may therefore finish with output similar to:

``` text
[RUN 1] RESULTS
-------------------------------------------------------------------------------
 Class error : 11.4285714286 %
 Accuracy    : 0.8857142857
 Precision   : 0.9130434783
 Recall      : 0.8750000000
 F1 score    : 0.8936170213
 Time        : 00:01:12
-------------------------------------------------------------------------------
```

At the end of all repetitions, the total execution time is also
displayed.

------------------------------------------------------------------------

# Dataset Path

The default dataset directory is:

``` text
$HOME/Desktop/ERGASIES/FeatureConstruction2/datasets/tenfolding
```

It can be changed without modifying the script by setting `DATAPATH`.

Example:

``` bash
DATAPATH=/home/user/datasets \
./runmodel_30.sh ionosphere_1
```

The script then looks for:

``` text
/home/user/datasets/ionosphere_1.train
/home/user/datasets/ionosphere_1.test
```

------------------------------------------------------------------------

# QGenClass Executable Path

The default executable is:

``` text
./QGenClass
```

A different executable can be selected using the `PROGRAM` environment
variable.

Example:

``` bash
PROGRAM=/home/user/QGenClass/build/QGenClass \
./runmodel_30.sh ionosphere_1
```

------------------------------------------------------------------------

# Comparing Local Search Methods

One of the main purposes of the script is to make different local-search
strategies easy to compare under the same experimental conditions.

For example:

``` bash
./runmodel_30.sh ionosphere_1 30 1 none class

./runmodel_30.sh ionosphere_1 30 1 mutateWorst class

./runmodel_30.sh ionosphere_1 30 1 targeted class

./runmodel_30.sh ionosphere_1 30 1 targetedWorst class

./runmodel_30.sh ionosphere_1 30 1 targetedBest class

./runmodel_30.sh ionosphere_1 30 1 constants class
```

Because all commands use the same base seed and number of repetitions,
the methods are evaluated with corresponding seed sequences.

The resulting summary files can then be compared.

------------------------------------------------------------------------

# Comparing Fitness Methods

The same local-search algorithm can also be evaluated with different
fitness methods.

For example:

``` bash
./runmodel_30.sh ionosphere_1 30 1 targeted class

./runmodel_30.sh ionosphere_1 30 1 targeted mean

./runmodel_30.sh ionosphere_1 30 1 targeted macroF1

./runmodel_30.sh ionosphere_1 30 1 targeted weightedF1
```

------------------------------------------------------------------------

# Recommended Experimental Workflow

For a quick test, first use a small number of repetitions:

``` bash
POP_GENS=100 \
./runmodel_30.sh ionosphere_1 2 1 targeted class
```

After confirming that the configuration works correctly, run the full
experiment:

``` bash
./runmodel_30.sh ionosphere_1 30 1 targeted class
```

For comparing local-search methods, keep all parameters and seeds
unchanged and modify only `LOCAL_METHOD`.

For example:

``` bash
./runmodel_30.sh ionosphere_1 30 1 targeted class
./runmodel_30.sh ionosphere_1 30 1 targetedWorst class
./runmodel_30.sh ionosphere_1 30 1 targetedBest class
./runmodel_30.sh ionosphere_1 30 1 constants class
```

This produces a more controlled comparison because every method is
evaluated using the same sequence of random seeds.

------------------------------------------------------------------------

# Error Checking

Before starting the experiments, the script verifies:

-   that the number of repetitions is valid;
-   that the base seed is valid;
-   that the requested local-search method is supported by the script;
-   that the requested fitness method is supported by the script;
-   that the QGenClass executable exists and is executable;
-   that the training file exists;
-   that the test file exists.

If QGenClass exits with a non-zero status, the experiment stops and the
corresponding log file is reported.

The script also stops if the final QGenClass metric line cannot be found
or parsed.

------------------------------------------------------------------------

# Quick Reference

## Default run

``` bash
./runmodel_30.sh ionosphere_1
```

## Targeted search

``` bash
./runmodel_30.sh ionosphere_1 30 1 targeted class
```

## Targeted worst-class search

``` bash
./runmodel_30.sh ionosphere_1 30 1 targetedWorst class
```

## Best-improvement targeted search

``` bash
./runmodel_30.sh ionosphere_1 30 1 targetedBest class
```

## Numerical-constant optimization

``` bash
./runmodel_30.sh ionosphere_1 30 1 constants class
```

## Larger population

``` bash
POP_COUNT=1000 \
./runmodel_30.sh ionosphere_1
```

## More generations

``` bash
POP_GENS=5000 \
./runmodel_30.sh ionosphere_1
```

## Stronger targeted search

``` bash
TARGETED_ITERATIONS=100 \
TARGETED_STAGNATION_LIMIT=10 \
TARGETED_BURST_ITERATIONS=500 \
./runmodel_30.sh ionosphere_1 30 1 targetedBest class
```

## Enable SMOTE

``` bash
ENABLE_SMOTE=yes \
SMOTE_K=5 \
./runmodel_30.sh ionosphere_1
```

------------------------------------------------------------------------

## Notes

The script documents the command-line options and output format used by
the current QGenClass experiment runner. If the QGenClass command-line
parser is changed, the corresponding option names in the script should
be updated as well.

The aggregate metrics are derived from the final `CLASS_ERROR`,
`PRECISION`, `RECALL`, and `F1SCORE` line printed by QGenClass.
Therefore, changes to that output format may require updating the
metric-parsing section of the script.
