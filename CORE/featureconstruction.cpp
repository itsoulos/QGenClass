#include <CORE/featureconstruction.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <map>

// FcCon-inspired expression construction; bounded arithmetic avoids NaN/Inf.
double FeatureConstruction::evaluate(const Data &x, const ConstructedFeature &f) const {
    const double a=x[f.a], b=x[f.b];
    double v=0;
    switch(f.operation) {
    case 0: v=a*b; break;
    case 1: v=a-b; break;
    case 2: v=a+b; break;
    case 3: v=std::sin(a); break;
    case 4: v=std::cos(a); break;
    case 5: v=a/(1.0+std::fabs(b)); break;
    case 6: v=std::tanh(a); break;
    default: v=0;
    }
    v*=f.scale;
    if(!std::isfinite(v)) return 0.0;
    return std::max(-100.0,std::min(100.0,v));
}
void FeatureConstruction::fit(const Dataset &train, int number, int seed) {
    selected.clear();
    inputDimension=train.dimension();
    if(number<=0 || inputDimension<=0 || train.count()<2) return;
    const int n=train.count();
    std::vector<Data> x;
    Data y;
    x.reserve(n); y.reserve(n);
    for(int i=0;i<n;++i) { x.push_back(train.getXPoint(i)); y.push_back(train.getYPoint(i)); }
    std::map<double,int> classIndex;
    for(double label:y) if(!classIndex.count(label)) classIndex[label]=static_cast<int>(classIndex.size());
    const int classes=static_cast<int>(classIndex.size());
    if(classes<2) return;
    std::mt19937 rng(static_cast<unsigned>(seed));
    std::uniform_int_distribution<int> variable(0,inputDimension-1);
    std::uniform_int_distribution<int> operation(0,6);
    const int trials=std::max(300,number*100);
    struct Candidate {double score; ConstructedFeature feature;};
    std::vector<Candidate> candidates;
    std::set<std::string> seen;
    for(int t=0;t<trials;++t) {
        ConstructedFeature f{variable(rng),variable(rng),operation(rng),1.0,""};
        if(f.operation==0 || f.operation==2) if(f.a>f.b) std::swap(f.a,f.b);
        char expr[128];
        switch(f.operation) {
        case 0: snprintf(expr,sizeof(expr),"x%d*x%d",f.a+1,f.b+1); break;
        case 1: snprintf(expr,sizeof(expr),"x%d-x%d",f.a+1,f.b+1); break;
        case 2: snprintf(expr,sizeof(expr),"x%d+x%d",f.a+1,f.b+1); break;
        case 3: snprintf(expr,sizeof(expr),"sin(x%d)",f.a+1); break;
        case 4: snprintf(expr,sizeof(expr),"cos(x%d)",f.a+1); break;
        case 5: snprintf(expr,sizeof(expr),"x%d/(1+abs(x%d))",f.a+1,f.b+1); break;
        default: snprintf(expr,sizeof(expr),"tanh(x%d)",f.a+1);
        }
        f.expression=expr;
        if(!seen.insert(f.expression).second) continue;
        std::vector<double> sums(classes,0.0), sums2(classes,0.0);
        std::vector<int> counts(classes,0);
        double total=0, total2=0;
        for(int i=0;i<n;++i) {
            const double v=evaluate(x[i],f);
            const int c=classIndex[y[i]];
            sums[c]+=v; sums2[c]+=v*v; ++counts[c];
            total+=v; total2+=v*v;
        }
        const double mean=total/n;
        const double variance=std::max(0.0,total2/n-mean*mean);
        if(variance<1e-12) continue;
        double between=0;
        for(int c=0;c<classes;++c) if(counts[c]) {
            const double d=sums[c]/counts[c]-mean;
            between+=counts[c]*d*d/n;
        }
        const double score=between/(variance+1e-12);
        candidates.push_back({score,f});
    }
    std::stable_sort(candidates.begin(),candidates.end(),[](const Candidate &a,const Candidate &b){return a.score>b.score;});
    for(int i=0;i<number && i<static_cast<int>(candidates.size());++i)
        selected.push_back(candidates[i].feature);
}
void FeatureConstruction::transform(Dataset &dataset) const {
    if(selected.empty() || dataset.count()==0) return;
    if(dataset.dimension()!=inputDimension) throw std::runtime_error("Feature construction: input dimension mismatch");
    Matrix transformed;
    Data labels;
    transformed.reserve(dataset.count()); labels.reserve(dataset.count());
    for(int i=0;i<dataset.count();++i) {
        Data row=dataset.getXPoint(i);
        for(const auto &f:selected) row.push_back(evaluate(row,f));
        transformed.push_back(std::move(row));
        labels.push_back(dataset.getYPoint(i));
    }
    dataset.setData(transformed,labels);
}
void FeatureConstruction::print() const {
    printf("FEATURE_CONSTRUCTION selected=%zu original_dimension=%d\n",selected.size(),inputDimension);
    for(size_t i=0;i<selected.size();++i)
        printf("  z%zu = %s\n",i+1,selected[i].expression.c_str());
}
