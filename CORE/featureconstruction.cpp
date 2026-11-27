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
    case 7: return a > f.threshold ? 1.0 : 0.0;
    case 8: return a > f.threshold && a <= f.upper ? 1.0 : 0.0;
    case 9: return a > f.threshold && b > f.upper ? 1.0 : 0.0;
    case 10: v=a-f.threshold; break;
    case 11: v=std::fabs(a-f.threshold); break;
    case 12: v=std::max(0.0,a-f.threshold); break;
    case 13: v=std::max(0.0,f.threshold-a); break;
    default: v=0;
    }
    v*=f.scale;
    if(!std::isfinite(v)) return 0.0;
    if(f.operation>=10) return v;
    return std::max(-100.0,std::min(100.0,v));
}
void FeatureConstruction::fit(const Dataset &train, int number, int seed, const std::string &method, int thresholds) {
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
    if(method == "gini" || method == "gini_continuous" || method == "gini_hybrid") {
        const bool binary = method != "gini_continuous";
        const bool continuous = method != "gini";
        // Rank binary partitions by weighted Gini reduction.
        std::vector<int> labels(n), totals(classes,0);
        for(int i=0;i<n;++i) ++totals[labels[i]=classIndex.at(y[i])];
        auto impurity = [](const std::vector<int>& counts, int size) {
            if(!size) return 0.0;
            double g=1.0;
            for(int c:counts) { double p=double(c)/size; g-=p*p; }
            return g;
        };
        const double parent=impurity(totals,n);
        auto gain = [&](const std::vector<int>& left, int size) {
            std::vector<int> right=totals;
            for(int c=0;c<classes;++c) right[c]-=left[c];
            return parent-(size*impurity(left,size)+(n-size)*impurity(right,n-size))/n;
        };
        auto add = [&](ConstructedFeature f) {
            if(!seen.insert(f.expression).second) return;
            if(f.operation>=10) {
                std::vector<std::pair<double,int>> values;
                values.reserve(n);
                for(int i=0;i<n;++i) values.emplace_back(evaluate(x[i],f),labels[i]);
                std::sort(values.begin(),values.end());
                std::vector<int> left(classes,0);
                double best=0.0;
                for(int i=0;i+1<n;++i) {
                    ++left[values[i].second];
                    if(values[i].first==values[i+1].first) continue;
                    best=std::max(best,gain(left,i+1));
                }
                if(best>0.0) candidates.push_back({best,f});
                return;
            }
            std::vector<int> counts(classes,0); int size=0;
            for(int i=0;i<n;++i) if(evaluate(x[i],f)>0.5) { ++counts[labels[i]]; ++size; }
            if(size && size<n) candidates.push_back({gain(counts,size),f});
        };
        std::vector<ConstructedFeature> splits;
        for(int j=0;j<inputDimension;++j) {
            std::vector<std::pair<double,int>> sorted;
            for(int i=0;i<n;++i) if(std::isfinite(x[i][j])) sorted.emplace_back(x[i][j],labels[i]);
            std::sort(sorted.begin(),sorted.end());
            std::vector<int> left(classes,0);
            std::vector<std::pair<double,double>> ranked;
            for(size_t i=0;i+1<sorted.size();++i) {
                ++left[sorted[i].second];
                if(sorted[i].first==sorted[i+1].first) continue;
                double a=sorted[i].first,b=sorted[i+1].first;
                double t=a/2+b/2; if(t>=b || t<a) t=a;
                ranked.emplace_back(gain(left,int(i+1)),t);
            }
            std::sort(ranked.rbegin(),ranked.rend());
            std::vector<double> ts;
            for(int k=0;k<thresholds && k<int(ranked.size());++k) {
                double t=ranked[k].second; ts.push_back(t);
                char expr[160]; snprintf(expr,sizeof(expr),"x%d>%.17g",j+1,t);
                ConstructedFeature f{j,j,7,1.0,expr}; f.threshold=t;
                if(binary) { add(f); splits.push_back(f); }
                if(continuous) {
                    const char* formats[]={"(x%d-(%.17g))","abs(x%d-(%.17g))","max(0,x%d-(%.17g))","max(0,-(x%d-(%.17g)))"};
                    for(int op=10;op<=13;++op) {
                        snprintf(expr,sizeof(expr),formats[op-10],j+1,t);
                        ConstructedFeature cf{j,j,op,1.0,expr}; cf.threshold=t; add(cf);
                    }
                }
            }
            std::sort(ts.begin(),ts.end());
            if(binary) for(size_t a=0;a<ts.size();++a) for(size_t b=a+1;b<ts.size();++b) {
                char expr[200]; snprintf(expr,sizeof(expr),"(x%d>%.17g && x%d<=%.17g)",j+1,ts[a],j+1,ts[b]);
                ConstructedFeature f{j,j,8,1.0,expr}; f.threshold=ts[a]; f.upper=ts[b]; add(f);
            }
        }
        if(!splits.empty()) {
            std::uniform_int_distribution<size_t> pick(0,splits.size()-1);
            for(int i=0;i<trials;++i) {
                auto a=splits[pick(rng)], b=splits[pick(rng)];
                if(a.a==b.a) continue;
                if(a.a>b.a) std::swap(a,b);
                ConstructedFeature f{a.a,b.a,9,1.0,"("+a.expression+" && "+b.expression+")"};
                f.threshold=a.threshold; f.upper=b.threshold; add(f);
            }
        }
    } else if(method == "standard") {
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
    } else throw std::runtime_error("Unknown feature construction method");
    std::stable_sort(candidates.begin(),candidates.end(),[](const Candidate &a,const Candidate &b){return a.score>b.score;});
    if(method=="gini_continuous" || method=="gini_hybrid") {
        // Avoid near-identical constructed columns; reserve both families in hybrid.
        std::vector<Data> chosen;
        auto choose = [&](int family, int limit) {
            int added=0;
            for(const auto &c:candidates) {
                if(added>=limit || int(selected.size())>=number) break;
                const auto &f=c.feature;
                if(family>=0 && int(f.operation>=10)!=family) continue;
                Data v; v.reserve(n);
                double scale=0;
                for(const auto &row:x) { v.push_back(evaluate(row,f)); scale=std::max(scale,std::fabs(v.back())); }
                if(scale==0) continue;
                double mean=0; for(double &z:v) { z/=scale; mean+=z/n; }
                double norm=0; for(double &z:v) { z-=mean; norm+=z*z; }
                if(norm<=1e-24) continue;
                for(double &z:v) z/=std::sqrt(norm);
                bool redundant=false;
                for(const auto &old:chosen) {
                    double dot=0; for(int i=0;i<n;++i) dot+=old[i]*v[i];
                    if(std::fabs(dot)>0.999) { redundant=true; break; }
                }
                if(redundant) continue;
                chosen.push_back(std::move(v)); selected.push_back(f); ++added;
            }
        };
        if(method=="gini_hybrid") { choose(0,number/2); choose(1,number-number/2); }
        choose(-1,number-int(selected.size()));
    } else {
        for(int i=0;i<number && i<static_cast<int>(candidates.size());++i)
            selected.push_back(candidates[i].feature);
    }
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
