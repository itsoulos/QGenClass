# QGenClass feature construction — experimental stage 1

## Build

```bash
qmake QGenClass.pro
make -j$(nproc)
```

## Usage

Original behavior (default):

```bash
./QGenClass --train_file=TRAIN --test_file=TEST --mode=standard
```

Feature construction:

```bash
./QGenClass --train_file=TRAIN --test_file=TEST --mode=feature_construction --features=5 --random_seed=1
```

Keep any existing QGenClass flags unchanged. The new mode generates a deterministic seeded pool of bounded arithmetic/nonlinear feature expressions, ranks them using training-set class separation, and appends the top candidates to both train and test data before constructing ClassProgram. The original QGenClass grammar and fitness remain in use.

**Scope:** This is an initial FcCon-inspired feature-construction prototype, **not** an integration of FcCon's Mapper/GE evolution. It does not jointly evolve the feature expressions and the classifier. No training labels from the test set are used for feature selection. The generated C/Python classifier export has NOT been extended to calculate the constructed features; it should not be used standalone in feature-construction mode. Full C++ compilation and runtime testing could not be performed here because Qt development tools/headers are unavailable.

## Running through the existing scripts

The original behavior is unchanged by default:

    ./runmodel.sh ionosphere 1

Enable feature construction with five features:

    MODE=feature_construction FEATURES=5 ./runmodel.sh ionosphere 1

The improved runner supports the same environment variables:

    MODE=feature_construction FEATURES=5 ./runmodel_improved.sh ionosphere 1 targeted class

For the original classifier explicitly use `MODE=standard`.
