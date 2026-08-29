#ifndef __CLASSPROGRAM__H
#define __CLASSPROGRAM__H

#include <GE/program.h>
#include <GE/cprogram.h>
#include <GE/rule.h>

#include <CORE/dataset.h>

#include <vector>
#include <string>

using namespace std;

#define FITNESS_CLASS 1
#define FITNESS_AVERAGE 2
#define FITNESS_SQUARED 4
#define FITNESS_MIXED 8
#define FITNESS_MEAN 16
#define FITNESS_MACRO_F1 32
#define FITNESS_WEIGHTED_F1 64

typedef vector<double> Data;

// ============================================================
// CLASS PROGRAM
// ============================================================

class ClassProgram : public Program
{
private:

    Dataset *trainSet;

    Dataset *testSet;

    Matrix trainx;

    Data trainy;

    vector<double> vclass;

    vector<string> pstring;

    vector<int> pgenome;

    Cprogram *program;

    vector<double> mapper;

    int dimension;

    int nclass;

    Data outy;

    int fitness_mode =
        FITNESS_CLASS;

    double class_percent =
        1.0;

    double average_percent =
        0.0;

    double squared_percent =
        0.0;

    Data realCached;

    Data estCached;

public:

    // ========================================================
    // CONSTRUCTOR
    // ========================================================

    ClassProgram(
        Dataset *tr,
        Dataset *tt
        );

    // ========================================================
    // FITNESS CONFIGURATION
    // ========================================================

    void setFitnessMode(
        int m
        );

    void setFitnessPercentages(
        double p1,
        double p2,
        double p3
        );

    // ========================================================
    // PHENOTYPE / OUTPUT
    // ========================================================

    string printF(
        vector<int> &genome
        );

    void printPython(
        vector<int> &genome,
        std::string outname =
        "classifier.py"
        );

    void printC(
        vector<int> &genome,
        std::string outname =
        "classifier.h"
        );

    // ========================================================
    // TARGETED GENOTYPE MUTATION SUPPORT
    // ========================================================

    /**
     * @brief Creates a complete codon trace for the classifier
     * chromosome.
     *
     * QGenClass internally splits the chromosome into parts,
     * one part for each class expression/rule.
     *
     * Rule::printRule() produces codon positions relative to
     * each local sub-genome.
     *
     * This method converts those local positions to positions
     * in the complete classifier chromosome.
     *
     * The resulting trace can later be used for targeted
     * genotype local search.
     *
     * Examples of targeted modifications:
     *
     *   cos  -> sin
     *   +    -> *
     *   x2   -> x7
     *   <    -> >=
     *
     * @param genome Complete classifier chromosome.
     * @param trace  Output vector containing meaningful codon
     *               decisions.
     */
    void getCodonTrace(
        vector<int> &genome,
        vector<CodonTrace> &trace
        );

    // ========================================================
    // MAPPER
    // ========================================================

    int findMapper(
        double x
        );

    // ========================================================
    // FITNESS
    // ========================================================

    virtual double fitness(
        vector<int> &genome
        );

    double getClassError(
        vector<int> &genome
        );

    // ========================================================
    // OUTPUTS
    // ========================================================

    void getOutputs(
        Dataset *t,
        vector<double> &real,
        vector<double> &est
        );

    void getOutputs(
        vector<double> &real,
        vector<double> &est
        );

    // ========================================================
    // CLASS INFORMATION
    // ========================================================

    int getClass() const;

    /**
     * @brief Returns number of input dimensions.
     */
    int getDimension() const;

    /**
     * @brief Returns the number of classes.
     *
     * This is useful for targeted genotype local search and
     * debugging.
     */
    int getClassCount() const
    {
        return nclass;
    }

    /**
     * @brief Returns the class values used by the classifier.
     *
     * The vector is returned by const reference so that no
     * unnecessary copy is created.
     */
    const vector<double> &getClasses() const
    {
        return vclass;
    }

    // ========================================================
    // CLASSIFICATION METRICS
    // ========================================================

    void getPrecisionAndRecall(
        double &precision,
        double &recall,
        double &macroF1,
        double &weightedF1,
        double &gmean
        );

    void getPrecisionAndRecall(
        Dataset *t,
        double &precision,
        double &recall,
        double &macroF1,
        double &weightedF1,
        double &gmean
        );

    // ========================================================
    // ERROR PER CLASS
    // ========================================================

    /**
     * @brief Returns the error per class for chromosome g.
     *
     * @param g Input chromosome.
     * @param x Output vector containing error per class.
     */
    void getErrorPerClass(
        vector<int> &g,
        vector<double> &x
        );

    // ========================================================
    // DESTRUCTOR
    // ========================================================

    ~ClassProgram();
};

#endif
