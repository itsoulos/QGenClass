# QGenClass unified experiment CLI

`runmodel.sh` and `runmodel_improved.sh` now use the same FcCon-style `--name value` options. All parameters registered by QGenClass `main.cpp` are exposed, except `--help` (handled by script), train/test file arguments and random seed (mapped to friendlier names).

Examples:

```bash
./runmodel.sh --dataset ionosphere --mode standard --seed 1
./runmodel.sh --dataset ionosphere --mode feature_construction --features 5 --seed 1
./runmodel.sh --dataset ionosphere --mode feature_construction --features 10 --runs 10 --generations 500 --chromosomes 500 --length 200 --local targeted --fitness class --crossover targeted --crossitems 10 --localitems 20 --localgens 50
./runmodel.sh --help
```

Notes: default mode is `standard`. Existing QGenClass executable C++ arguments are unchanged. `runmodel_30.sh` and `run.sh` remain legacy positional scripts, unchanged. All `--name` options also accept `--name=value`. Log files go to `results/` by default. To reproduce the original `runmodel.sh` settings, use the script defaults; the new script also supports explicit `--termination` and classifier output filenames. The feature-construction implementation itself is unchanged.
