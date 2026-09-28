#ifndef __POPULATION__H

#include <GE/program.h>

#include <random>
#include <string>
#include <vector>

using namespace std;

// ============================================================
// LOCAL SEARCH TYPES
// ============================================================

#define GELOCAL_NONE        0x0
#define GELOCAL_CROSSOVER   0x1
#define GELOCAL_MUTATE      0x2
#define GELOCAL_BFGS        0x4
#define GELOCAL_SIMAN       0x8
#define GELOCAL_DE          0x16
#define GELOCAL_HILL        0x32
#define GELOCAL_GD          0x64
#define GELOCAL_ADAM        0x128

// ============================================================
// NEW:
// TARGETED GENOTYPE LOCAL SEARCH
//
// This method modifies meaningful codons according to the
// grammar trace:
//
// FUNCTION:
//     cos -> sin
//     sin -> exp
//     ...
//
// BINARY OPERATOR:
//     + -> *
//     - -> /
//
// BOOLEAN OPERATOR:
//     < -> >=
//     == -> !=
//
// VARIABLE:
//     x2 -> x7
//
// The modification is applied directly to the chromosome.
// ============================================================

#define GELOCAL_TARGETED    0x256

// ============================================================
// POPULATION
// ============================================================

/**
 * @brief The Population class holds the current GE population.
 *
 * Mutation, selection, crossover, and local-search operators
 * are implemented here.
 */
class Population
{
private:
    // ============================================================
    // TARGETED SEARCH STATISTICS
    // ============================================================

    long targetedFunctionAttempts = 0;
    long targetedFunctionAccepted = 0;

    long targetedBinaryAttempts = 0;
    long targetedBinaryAccepted = 0;

    long targetedBooleanAttempts = 0;
    long targetedBooleanAccepted = 0;

    long targetedVariableAttempts = 0;
    long targetedVariableAccepted = 0;


    // ============================================================
    // STAGNATION CONTROL
    // ============================================================

    int targetedStagnationLimit = 20;

    int targetedBurstIterations = 200;

    int generationsWithoutImprovement = 0;

    double previousBestFitness = -1e+100;


    // ============================================================
    // LOCAL CROSSOVER METHOD
    // ============================================================

    string crossMethod = "standard";

    // Number of candidate crossover attempts.
    int targetedCrossIterations = 50;

    // Fraction of the sorted population used as donor pool.
    // 0.20 means donors come from the best 20%.
    double targetedCrossEliteFraction = 0.20;

    // Maximum number of codons copied in one semantic block.
    int targetedCrossMaxBlock = 16;

    // ============================================================
    // TARGETED ADAPTIVE WEIGHTS
    // ============================================================

    double targetedFunctionWeight = 1.0;
    double targetedBinaryWeight = 1.0;
    double targetedBooleanWeight = 1.0;
    double targetedVariableWeight = 1.0;
    // ========================================================
    // POPULATION ARRAYS
    // ========================================================

    int **children;

    int **trialx;

    int **genome;

    int *valid;

    double *fitness_array;

    // ========================================================
    // GE PARAMETERS
    // ========================================================

    double mutation_rate;

    double selection_rate;

    int genome_count;

    int genome_size;

    int generation;

    Program *program;

    // ========================================================
    // BASIC EVOLUTIONARY OPERATORS
    // ========================================================

    void select();

    void crossover();

    void mutate();

    void calcFitnessArray();

    void replaceWorst();

    // ========================================================
    // ELITISM
    // ========================================================

    int elitism;

    // ========================================================
    // LOCAL SEARCH
    // ========================================================

    string localMethod = "none";

    /**
     * @brief Applies the selected local search to chromosome
     *        at population position gpos.
     *
     * Existing values include methods such as:
     *
     * none
     * crossover
     * mutate
     * siman
     * gd
     * adam
     *
     * We additionally support:
     *
     * targeted
     */
    void localSearch(
        int gpos
        );

    // ========================================================
    // LOCAL SEARCH FREQUENCY
    // ========================================================

    int crossitems = 10;

    int localitems = 10;

    int localgens = 100;

    // ========================================================
    // NEW:
    // NUMBER OF TARGETED LOCAL SEARCH ITERATIONS
    // ========================================================

    int targetedIterations = 100;

    // ========================================================
    // NEW:
    // TARGETED GENOTYPE SUPPORT
    // ========================================================

    /**
     * @brief Produces a codon value that selects a requested
     *        grammar production while remaining close to the
     *        original integer codon.
     *
     * We want:
     *
     *     newCodon % ruleCount == desiredRule
     *
     * Example:
     *
     * oldCodon = 13
     * ruleCount = 4
     *
     * 13 % 4 = 1
     *
     * If production 1 corresponds to cos and production 2
     * corresponds to sin, we can construct:
     *
     * newCodon = 14
     *
     * because:
     *
     * 14 % 4 = 2
     *
     * @param oldCodon Current integer codon.
     * @param ruleCount Number of grammar alternatives.
     * @param desiredRule Required production index.
     *
     * @return New integer codon.
     */
    static int codonForRule(
        int oldCodon,
        int ruleCount,
        int desiredRule
        );

public:

    // ========================================================
    // CONSTRUCTOR
    // ========================================================

    Population(int gcount,int gsize,Program *p);

    // ========================================================
    // BASIC SETTINGS
    // ========================================================

    void    setCrossItems(int g);
    void    setLocalItems(int g);
    void    setLocalGens(int g);
    double  fitness(vector<int> &g);
    void    setElitism(int s);
    void    setLocalMethod(string s);

    // ========================================================
    // NEW:
    // TARGETED LOCAL SEARCH SETTINGS
    // ========================================================

    /**
     * @brief Sets the number of targeted genotype local-search
     *        iterations.
     *
     * Example:
     *
     *     population.setTargetedIterations(100);
     */
    void setTargetedIterations(int n);

    /**
     * @brief Performs targeted genotype local search on the
     *        chromosome stored at population position pos.
     *
     * The function will:
     *
     * 1. Copy the current chromosome.
     * 2. Obtain a CodonTrace from ClassProgram.
     * 3. Select a meaningful grammar decision.
     * 4. Change only the associated codon.
     * 5. Evaluate the modified chromosome.
     * 6. Keep the modification if fitness improves.
     *
     * Example transformations:
     *
     *     cos -> sin
     *     +   -> *
     *     <   -> >=
     *     x3  -> x7
     *
     * @param pos Position of chromosome in population.
     * @param iterations Number of local-search iterations.
     */
    void targetedLocalSearch(int pos,int iterations = 100);

    // ========================================================
    // POPULATION INFORMATION
    // ========================================================

    int getGeneration() const;
    int getCount() const;
    int getSize() const;
    // ========================================================
    // EVOLUTION
    // ========================================================

    void nextGeneration();
    // ========================================================
    // MUTATION / SELECTION PARAMETERS
    // ========================================================

    void setMutationRate(double r);
    void setSelectionRate(double r);
    double getSelectionRate() const;
    double getMutationRate() const;

    // ========================================================
    // BEST SOLUTION
    // ========================================================

    double getBestFitness() const;
    double evaluateBestFitness();
    vector<int> getBestGenome() const;

    // ========================================================
    // RESET
    // ========================================================

    void reset();
    // ========================================================
    // EXISTING INTEGER LOCAL SEARCH METHODS
    // ========================================================

    vector<int> discreteGradient(vector<int>& x);

    vector<int> discreteStep(vector<int>& x,vector<int>& grad);

    void integerLocalSearch(vector<int> &x,int maxSteps = 20);

    // ========================================================
    // INTEGER ADAM
    // ========================================================

    vector<int> integerAdam(
        vector<int> x,
        int steps = 20,
        double alpha = 0.5,
        double beta1 = 0.9,
        double beta2 = 0.999,
        double eps = 1e-8
        );

    // ========================================================
    // NEIGHBOR GENERATION
    // ========================================================

    vector<int> neighbor(
        const vector<int>& x,
        int stepSize = 1
        );

    // ========================================================
    // SIMULATED ANNEALING
    // ========================================================

    vector<int> simulatedAnnealing(
        vector<int> &x,
        double T0 = 100.0,
        double Tmin = 1e-3,
        double alpha = 0.95,
        int iterPerTemp = 20
        );

    // ========================================================
    // EXISTING MEMETIC OPERATORS
    // ========================================================

    void crossItem(int pos);
    void mutateItem(int pos);

    /**
     * @brief Performs mutation on chromosome at population
     *        position pos. The mutation is restricted to the
     *        segment corresponding to classIndex.
     *
     * @param pos Population chromosome position.
     * @param classIndex Class segment to modify.
     */
    void mutateItemAtClass(int pos,int classIndex);


    void targetedWorstLocalSearch(int pos,int iterations = 100);
    void targetedBestLocalSearch(int pos,int iterations = 100);

    void setTargetedStagnationLimit(int n);
    void setTargetedBurstIterations(int n);
    void printTargetedStatistics() const;
    void constantsLocalSearch(int pos,int iterations = 100);


    void setCrossMethod(string method);
    void setTargetedCrossIterations(int n);
    void setTargetedCrossEliteFraction(double x);
    void setTargetedCrossMaxBlock(int n);
    void targetedCrossItem(int pos,int iterations = 50);
    void targetedCrossWorstItem(int pos,int iterations = 50);
    // ========================================================
    // DESTRUCTOR
    // ========================================================
    ~Population();
};

#define __POPULATION__H

#endif
