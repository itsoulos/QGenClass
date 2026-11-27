#ifndef QGEN_FEATURECONSTRUCTION_H
#define QGEN_FEATURECONSTRUCTION_H
#include <CORE/dataset.h>
#include <string>
#include <vector>
struct ConstructedFeature {
    int a, b, operation;
    double scale;
    std::string expression;
    double threshold = 0.0, upper = 0.0;
};
// Training-only supervised selection of nonlinear features.
// Same stored transformations are applied to test data.
class FeatureConstruction {
    std::vector<ConstructedFeature> selected;
    int inputDimension = 0;
    double evaluate(const Data &x, const ConstructedFeature &f) const;
public:
    void fit(const Dataset &train, int number, int seed, const std::string &method = "standard", int thresholds = 5);
    void transform(Dataset &dataset) const;
    void print() const;
};
#endif
