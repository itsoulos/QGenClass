#include <GE/population.h>
#include <GE/classprogram.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

using namespace std;

// ============================================================
// GLOBAL SETTINGS
// ============================================================

int MAX_RULE = 256;

mt19937 gen(random_device{}());


static const char *codonTypeName(CodonType type)
{
    switch(type)
    {
    case CodonType::FUNCTION:
        return "FUNCTION";

    case CodonType::BINARY_OPERATOR:
        return "BINARY_OPERATOR";

    case CodonType::BOOLEAN_OPERATOR:
        return "BOOLEAN_OPERATOR";

    case CodonType::VARIABLE:
        return "VARIABLE";

    case CodonType::CONSTANT:
        return "CONSTANT";

    case CodonType::CLASS_OUTPUT:
        return "CLASS_OUTPUT";

    default:
        return "UNKNOWN";
    }
}
// ============================================================
// RANDOM UTILITIES
// ============================================================

static double targetedSuccessRate(long attempts,long accepted)
{
    /*
     * Laplace smoothing.
     *
     * Prevents mutation categories with very few attempts
     * from receiving zero probability.
     */

    return (accepted + 1.0)/(attempts + 2.0);
}
double randDouble(
    double a,
    double b)
{
    uniform_real_distribution<> dist(a, b);

    return dist(gen);
}

int randInt(
    int a,
    int b
    )
{
    uniform_int_distribution<> dist(a, b);

    return dist(gen);
}

// ============================================================
// NEIGHBOR
// ============================================================

vector<int> Population::neighbor(const vector<int>& x,int stepSize)
{
    vector<int> y = x;

    if (x.empty())return y;

    int i =randInt(0,static_cast<int>(x.size()) - 1);
    y[i] +=randInt(-stepSize,stepSize);
    if (y[i] < 0)y[i] = 0;
    return y;
}

// ============================================================
// SIMULATED ANNEALING
// ============================================================

vector<int> Population::simulatedAnnealing(
    vector<int> &x,
    double T0,
    double Tmin,
    double alpha,
    int iterPerTemp
    )
{
    double T = T0;
    vector<int> best = x;
    double bestVal =fitness(x);

    while (T > Tmin)
    {
        for (int k = 0;k < iterPerTemp;k++)
        {
            vector<int> y =neighbor(x);
            double fx =fitness(x);
            double fy =fitness(y);

            double delta =fabs(fy) -fabs(fx);

            if (delta < 0)
            {
                x = y;
            }
            else
            {
                double p =exp(-delta / T);
                if (randDouble(0,1) < p)
                {
                    x = y;
                }
            }

            double val =fitness(x);
            if (fabs(val) <fabs(bestVal))
            {
                best = x;
                bestVal = val;
            }
        }
        printf("T=%20.10lf BEST=%20.10lf\n",T,bestVal);
        T *= alpha;
    }

    return best;
}

// ============================================================
// POPULATION CONSTRUCTOR
// ============================================================

Population::Population(
    int gcount,
    int gsize,
    Program *p
    )
{
    elitism = 1;
    selection_rate = 0.1;
    mutation_rate = 0.1;
    genome_count = gcount;
    genome_size = gsize;
    generation = 0;
    program = p;

    previousBestFitness =-1e+100;
    generationsWithoutImprovement =0;
    genome = new int*[genome_count];
    children =new int*[genome_count];
    vector<int> g;
    g.resize(genome_size);

    ClassProgram *pp =(ClassProgram *)program;

    if (pp->getDimension() >255)
    {
        MAX_RULE =pp->getDimension() +1;
    }

    for (int i = 0;i < genome_count;i++)
    {
        genome[i] =new int[genome_size];
        children[i] =new int[genome_size];

        for (int j = 0;j < genome_size;j++)
        {
            g[j] =genome[i][j] =rand() %MAX_RULE;
        }
    }

    fitness_array =new double[genome_count];
}

// ============================================================
// RESET POPULATION
// ============================================================

void Population::reset()
{
    generation = 0;
    previousBestFitness =-1e+100;
    generationsWithoutImprovement =0;
    for (int i = 0;i < genome_count;i++)
    {
        for (int j = 0;j < genome_size;j++)
        {
            genome[i][j] =rand() %MAX_RULE;
        }
    }

    for (int i = 0;i < genome_count;i++)
    {
        fitness_array[i] =-1e+100;
    }
}

// ============================================================
// FITNESS
//
// IMPORTANT:
//
// Program::fitness() is a minimization objective.
//
// Population::fitness() negates it:
//
//     Population fitness = -Program fitness
//
// Therefore, inside Population:
//
//     LARGER FITNESS IS BETTER.
// ============================================================

double Population::fitness(vector<int> &g)
{
    return -program->fitness(g);
}

// ============================================================
// SELECTION
// ============================================================

void Population::select()
{
    int itemp[genome_size];

    for (int i = 0;i < genome_count;i++)
    {
        for (int j = 0;j <genome_count - 1;j++)
        {
            if (fitness_array[j + 1] >fitness_array[j])
            {
                double dtemp;
                dtemp =fitness_array[j];
                fitness_array[j] =fitness_array[j + 1];
                fitness_array[j + 1] =dtemp;

                memcpy(itemp,genome[j],genome_size *sizeof(int));
                memcpy(genome[j],genome[j + 1],genome_size *sizeof(int));

                memcpy(genome[j + 1],itemp,genome_size *sizeof(int));
            }
        }
    }
}

// ============================================================
// CROSSOVER
// ============================================================

void Population::crossover()
{
    int parent[2];
    int nchildren =static_cast<int>((1.0 -selection_rate) *genome_count);
    if (!(nchildren % 2 == 0))
    {
        nchildren++;
    }
    const int tournament_size =(genome_count <= 100)?4:20;
    int count_children = 0;

    while (1)
    {
        for (int i = 0;i < 2;i++)
        {
            double max_fitness =-1e+10;
            int max_index =-1;
            int r;
            for (int j = 0;j < tournament_size;j++)
            {
                r =rand() %genome_count;
                if (j == 0 ||fitness_array[r] >max_fitness)
                {
                    max_index =r;
                    max_fitness =fitness_array[r];
                }
            }
            parent[i] =max_index;
        }
        int pt1;
        pt1 =rand() %genome_size;
        memcpy(children[count_children],genome[parent[0]],pt1 *sizeof(int));

        memcpy(&children[count_children][pt1],&genome[parent[1]][pt1],(genome_size -pt1) *sizeof(int));

        memcpy(children[count_children +1],genome[parent[1]],pt1 *sizeof(int));

        memcpy(&children[count_children +1][pt1],&genome[parent[0]][pt1],(genome_size -pt1) *sizeof(int));

        count_children +=2;
        if (count_children >=nchildren)
        {
            break;
        }
    }

    vector<int> g;
    g.resize(genome_size);

    for (int i = 0;i < nchildren;i++)
    {
        memcpy(genome[genome_count -i -1],children[i],genome_size *sizeof(int));
    }
}

// ============================================================
// ELITISM
// ============================================================

void Population::setElitism(int s)
{
    elitism = s;
}

// ============================================================
// STANDARD MUTATION
// ============================================================

void Population::mutate()
{
    int start =elitism *static_cast<int>(genome_count *selection_rate);
    start =elitism;
    start =1;

    for (int i = start;i < genome_count;i++)
    {
        for (int j = 0;j < genome_size;j++)
        {
            double r =rand() *1.0 /RAND_MAX;
            if (r <mutation_rate)
            {
                genome[i][j] =rand() %MAX_RULE;
            }
        }
    }
}

// ============================================================
// FITNESS ARRAY
// ============================================================

void Population::calcFitnessArray()
{
    vector<int> g;
    g.resize(genome_size);

    double dmin =1e+100;

    for (int i = 0;i < genome_count;i++)
    {
        for (int j = 0;j < genome_size;j++)
        {
            g[j] =genome[i][j];
        }

        fitness_array[i] =fitness(g);

        if (fabs(fitness_array[i]) < dmin)
        {
            dmin =fabs(fitness_array[i]);
        }
        if ((i + 1) %50 ==0)
        {
            printf(" %d:%.5lg ",i + 1,dmin);
            fflush(stdout);
        }
    }
}

// ============================================================
// GET GENERATION
// ============================================================

int Population::getGeneration() const
{
    return generation;
}

// ============================================================
// GET COUNT
// ============================================================

int Population::getCount() const
{
    return genome_count;
}

// ============================================================
// GET SIZE
// ============================================================

int Population::getSize() const
{
    return genome_size;
}


void Population::setCrossMethod(string method)
{
    if(
        method == "standard"
        ||method == "targeted"
        ||method == "targetedWorst")
    {
        crossMethod = method;
    }
}

void Population::setTargetedCrossIterations(int n)
{
    if(n > 0)targetedCrossIterations = n;
}

void Population::setTargetedCrossEliteFraction(double x)
{
    if(x > 0.0 && x <= 1.0)targetedCrossEliteFraction = x;
}

void Population::setTargetedCrossMaxBlock(int n)
{
    if(n > 0)targetedCrossMaxBlock = n;
}


// ============================================================
// TARGETED SEMANTIC LOCAL CROSSOVER
// ============================================================

void Population::targetedCrossItem(int pos,int iterations)
{
    if(pos < 0 ||pos >= genome_count)return;
    if(iterations <= 0)return;
    ClassProgram *p =(ClassProgram *)program;

    if(p == nullptr) return;

    // --------------------------------------------------------
    // Current chromosome
    // --------------------------------------------------------

    vector<int> current(genome_size);

    for(int i=0;i<genome_size;i++)current[i] = genome[pos][i];

    double bestFitness =fitness(current);

    // --------------------------------------------------------
    // Elite donor pool
    //
    // Population has already been sorted by select().
    // --------------------------------------------------------

    int eliteCount =
        static_cast<int>(targetedCrossEliteFraction *genome_count);

    if(eliteCount < 2)eliteCount = 2;
    if(eliteCount > genome_count)eliteCount = genome_count;

    int accepted = 0;

    printf(
        "TARGETED_CROSS[%d] START fitness=%.10lf\n",pos,bestFitness);
    fflush(stdout);
    // ========================================================
    // TRIALS
    // ========================================================

    for(int trial=0;trial<iterations;trial++)
    {
        // ----------------------------------------------------
        // Target trace
        // ----------------------------------------------------

        vector<CodonTrace> targetTrace;

        p->getCodonTrace(current,targetTrace);
        if(targetTrace.empty())break;

        // ----------------------------------------------------
        // Keep useful semantic entries only
        // ----------------------------------------------------

        vector<CodonTrace> usableTarget;

        for(const CodonTrace &e : targetTrace)
        {
            if(
                e.type == CodonType::FUNCTION ||
                e.type == CodonType::BINARY_OPERATOR ||
                e.type == CodonType::BOOLEAN_OPERATOR ||
                e.type == CodonType::VARIABLE ||
                e.type == CodonType::CONSTANT
                )
            {
                usableTarget.push_back(e);
            }
        }

        if(usableTarget.empty())break;

        // ----------------------------------------------------
        // Pick semantic point in target
        // ----------------------------------------------------

        const CodonTrace targetEntry =
            usableTarget[rand() %usableTarget.size()];

        // ----------------------------------------------------
        // Pick donor from elite population
        // ----------------------------------------------------

        int donorPos = -1;

        for(int retry=0;retry<20;retry++)
        {
            int d =rand() %eliteCount;

            if(d != pos)
            {
                donorPos = d;
                break;
            }
        }

        if(donorPos < 0)continue;

        vector<int> donor(genome_size);

        for(int i=0;i<genome_size;i++)
            donor[i] = genome[donorPos][i];

        // ----------------------------------------------------
        // Donor trace
        // ----------------------------------------------------

        vector<CodonTrace> donorTrace;

        p->getCodonTrace(donor,donorTrace);

        if(donorTrace.empty()) continue;

        // ----------------------------------------------------
        // Find donor entries with SAME semantic type
        // ----------------------------------------------------

        vector<CodonTrace> compatible;

        for(const CodonTrace &e : donorTrace)
        {
            if(e.type ==targetEntry.type)
            {
                compatible.push_back(e);
            }
        }

        if(compatible.empty()) continue;

        const CodonTrace donorEntry =
            compatible[rand() %compatible.size()];

        if(
            targetEntry.genomePos < 0 ||
            targetEntry.genomePos >= genome_size ||
            donorEntry.genomePos < 0 ||
            donorEntry.genomePos >= genome_size
            )
            continue;

        // ====================================================
        // DETERMINE SMALL SEMANTIC BLOCK LENGTH
        // ====================================================

        int targetNext =genome_size;

        int donorNext =genome_size;

        /*
         * Find next active semantic codon after selected
         * position. This approximates the extent of the local
         * grammatical block.
         */

        for(const CodonTrace &e : targetTrace)
        {
            if(e.genomePos >targetEntry.genomePos
                && e.genomePos <targetNext)
            {
                targetNext =e.genomePos;
            }
        }

        for(const CodonTrace &e : donorTrace)
        {
            if(e.genomePos >donorEntry.genomePos &&
                e.genomePos <donorNext)
            {
                donorNext =e.genomePos;
            }
        }

        int targetLength =
            targetNext -targetEntry.genomePos;

        int donorLength =
            donorNext -donorEntry.genomePos;

        int blockLength = min(targetLength,donorLength);

        blockLength =min(blockLength,targetedCrossMaxBlock);

        if(blockLength < 1) blockLength = 1;

        /*
         * Do not cross genome boundaries.
         */

        blockLength =
            min(blockLength,genome_size -targetEntry.genomePos);

        blockLength =
            min(blockLength,genome_size -donorEntry.genomePos);

        if(blockLength <= 0) continue;

        // ====================================================
        // BUILD CANDIDATE
        // ====================================================

        vector<int> candidate =current;

        for(int k=0;k<blockLength;k++)
        {
            candidate[targetEntry.genomePos + k] =
                donor[donorEntry.genomePos + k];
        }

        double candidateFitness =fitness(candidate);

        // ====================================================
        // ACCEPT ONLY IMPROVEMENT
        // ====================================================

        if(candidateFitness >bestFitness)
        {
            printf(
                "TARGETED_CROSS[%d] "
                "trial=%d "
                "donor=%d "
                "type=%s "
                "targetPos=%d "
                "donorPos=%d "
                "length=%d "
                "fitness=%.10lf->%.10lf\n",
                pos,
                trial + 1,
                donorPos,
                codonTypeName(
                    targetEntry.type
                    ),
                targetEntry.genomePos,
                donorEntry.genomePos,
                blockLength,
                bestFitness,
                candidateFitness
                );

            fflush(stdout);

            current =candidate;
            bestFitness =candidateFitness;
            accepted++;
        }
    }

    // ========================================================
    // COPY BACK
    // ========================================================

    for(int i=0;i<genome_size;i++) genome[pos][i] = current[i];

    fitness_array[pos] = bestFitness;

    printf(
        "TARGETED_CROSS[%d] END "
        "fitness=%.10lf accepted=%d\n",
        pos,
        bestFitness,
        accepted
        );

    fflush(stdout);
}


// ============================================================
// TARGETED SEMANTIC CROSSOVER FOR WORST CLASS
// ============================================================

void Population::targetedCrossWorstItem(int pos,int iterations)
{
    if(pos < 0 ||pos >= genome_count)return;
    if(iterations <= 0)return;
    ClassProgram *p =(ClassProgram *)program;

    if(p == nullptr)return;

    const int classCount =p->getClass();

    if(classCount <= 1)return;

    vector<int> current(genome_size);

    for(int i=0;i<genome_size;i++)current[i] = genome[pos][i];

    double bestFitness =fitness(current);

    int eliteCount =
        static_cast<int>(targetedCrossEliteFraction *genome_count);

    if(eliteCount < 2)eliteCount = 2;

    if(eliteCount > genome_count) eliteCount = genome_count;

    int accepted = 0;

    printf(
        "TARGETED_CROSS_WORST[%d] START "
        "fitness=%.10lf\n",
        pos,
        bestFitness
        );

    fflush(stdout);

    for(int trial=0;trial<iterations;trial++)
    {
        // ====================================================
        // FIND CURRENT WORST CLASS
        // ====================================================

        vector<double> classError;

        p->getErrorPerClass(current,classError);

        if(classError.empty()) break;

        int worstClass = -1;
        double worstError =-1.0;

        for(int c=0;c<(int)classError.size();c++)
        {
            if(classError[c] >worstError)
            {
                worstError =classError[c];
                worstClass =c;
            }
        }

        if(worstClass < 0) break;

        // ====================================================
        // CLASS SEGMENT
        // ====================================================

        const int explicitRules = classCount - 1;
        const int partSize =genome_size /explicitRules;
        bool wholeGenome =(worstClass >=explicitRules);

        int segmentStart = 0;
        int segmentEnd =genome_size;
        if(!wholeGenome)
        {
            segmentStart = worstClass *partSize;
            segmentEnd =(worstClass + 1) *partSize;

            if(worstClass ==explicitRules - 1)
            {
                segmentEnd =genome_size;
            }
        }

        // ====================================================
        // TARGET TRACE
        // ====================================================

        vector<CodonTrace> targetTrace;
        p->getCodonTrace(current,targetTrace);

        vector<CodonTrace> usableTarget;
        for(const CodonTrace &e : targetTrace)
        {
            bool semantic =
                (
                    e.type == CodonType::FUNCTION ||
                    e.type == CodonType::BINARY_OPERATOR ||
                    e.type == CodonType::BOOLEAN_OPERATOR ||
                    e.type == CodonType::VARIABLE ||
                    e.type == CodonType::CONSTANT
                    );

            if(!semantic) continue;

            if(
                wholeGenome ||
                (e.genomePos >=segmentStart &&
                    e.genomePos <segmentEnd))
            {
                usableTarget.push_back(e);
            }
        }

        if(usableTarget.empty()) continue;

        const CodonTrace targetEntry =
            usableTarget[rand() %usableTarget.size()];

        // ====================================================
        // ELITE DONOR
        // ====================================================

        int donorPos = -1;

        for(int retry=0;retry<20;retry++)
        {
            int d =rand() %eliteCount;

            if(d != pos)
            {
                donorPos = d;
                break;
            }
        }

        if(donorPos < 0) continue;

        vector<int> donor(genome_size);

        for(int i=0;i<genome_size;i++)
            donor[i] =genome[donorPos][i];

        vector<CodonTrace> donorTrace;

        p->getCodonTrace(donor,donorTrace);

        // ====================================================
        // COMPATIBLE DONOR POSITIONS
        //
        // Same semantic type and, when there is an explicit
        // worst-class rule, same class segment.
        // ====================================================

        vector<CodonTrace> compatible;

        for(const CodonTrace &e : donorTrace)
        {
            if(e.type !=targetEntry.type)
            {
                continue;
            }

            if(
                wholeGenome ||
                (
                    e.genomePos >=segmentStart &&
                    e.genomePos <segmentEnd))
            {
                compatible.push_back(e);
            }
        }

        if(compatible.empty()) continue;

        const CodonTrace donorEntry =
            compatible[rand() %compatible.size()];

        // ====================================================
        // FIND BLOCK EXTENT
        // ====================================================

        int targetNext =
            wholeGenome?genome_size:segmentEnd;

        int donorNext =
            wholeGenome
                ?
                genome_size
                :
                segmentEnd;

        for(const CodonTrace &e : targetTrace)
        {
            if(
                e.genomePos >
                    targetEntry.genomePos &&
                e.genomePos <
                    targetNext
                )
            {
                targetNext =
                    e.genomePos;
            }
        }

        for(const CodonTrace &e : donorTrace)
        {
            if(
                e.genomePos >
                    donorEntry.genomePos &&
                e.genomePos <
                    donorNext
                )
            {
                donorNext =
                    e.genomePos;
            }
        }

        int blockLength =
            min(
                targetNext -
                    targetEntry.genomePos,
                donorNext -
                    donorEntry.genomePos
                );

        blockLength =
            min(
                blockLength,
                targetedCrossMaxBlock
                );

        if(blockLength < 1)
            blockLength = 1;

        if(!wholeGenome)
        {
            blockLength =
                min(
                    blockLength,
                    segmentEnd -
                        targetEntry.genomePos
                    );

            blockLength =
                min(
                    blockLength,
                    segmentEnd -
                        donorEntry.genomePos
                    );
        }

        blockLength =
            min(
                blockLength,
                genome_size -
                    targetEntry.genomePos
                );

        blockLength =
            min(
                blockLength,
                genome_size -
                    donorEntry.genomePos
                );

        if(blockLength <= 0)
            continue;

        // ====================================================
        // CROSS
        // ====================================================

        vector<int> candidate =
            current;

        for(
            int k=0;
            k<blockLength;
            k++
            )
        {
            candidate[
                targetEntry.genomePos + k
            ] =
                donor[
                    donorEntry.genomePos + k
            ];
        }

        double candidateFitness =
            fitness(candidate);

        if(
            candidateFitness >
            bestFitness
            )
        {
            printf(
                "TARGETED_CROSS_WORST[%d] "
                "trial=%d "
                "worstClass=%d "
                "error=%.4lf "
                "donor=%d "
                "type=%s "
                "targetPos=%d "
                "donorPos=%d "
                "length=%d "
                "fitness=%.10lf->%.10lf\n",
                pos,
                trial + 1,
                worstClass,
                worstError,
                donorPos,
                codonTypeName(
                    targetEntry.type
                    ),
                targetEntry.genomePos,
                donorEntry.genomePos,
                blockLength,
                bestFitness,
                candidateFitness
                );

            fflush(stdout);

            current =
                candidate;

            bestFitness =
                candidateFitness;

            accepted++;
        }
    }

    // ========================================================
    // COPY BACK
    // ========================================================

    for(int i=0;i<genome_size;i++)
        genome[pos][i] =
            current[i];

    fitness_array[pos] =
        bestFitness;

    printf(
        "TARGETED_CROSS_WORST[%d] END "
        "fitness=%.10lf accepted=%d\n",
        pos,
        bestFitness,
        accepted
        );

    fflush(stdout);
}


// ============================================================
// SET CROSS ITEMS
// ============================================================

void Population::setCrossItems(
    int g
    )
{
    if (g >= 0)
        crossitems = g;
}

// ============================================================
// SET LOCAL ITEMS
// ============================================================

void Population::setLocalItems(
    int g
    )
{
    if (g >= 0)
        localitems = g;
}

// ============================================================
// SET LOCAL GENERATIONS
// ============================================================

void Population::setLocalGens(
    int g
    )
{
    if (g >= 0)
        localgens = g;
}

// ============================================================
// NEW:
// TARGETED ITERATIONS
// ============================================================

void Population::setTargetedIterations(
    int n
    )
{
    if (n > 0)
        targetedIterations = n;
}

// ============================================================
// NEXT GENERATION
// ============================================================

void Population::nextGeneration()
{
    calcFitnessArray();
    select();

    // ============================================================
    // STAGNATION DETECTION
    // ============================================================

    double currentBest =fitness_array[0];
    if(currentBest >previousBestFitness +1e-12)
    {
        previousBestFitness =currentBest;
        generationsWithoutImprovement =0;
    }
    else
    {
        generationsWithoutImprovement++;
    }

    // --------------------------------------------------------
    // PERIODIC LOCAL SEARCH
    // --------------------------------------------------------

    if (localgens > 0 && (generation +1) %localgens ==0)
    {
        for (int i = 0;i < localitems;i++)
        {
            localSearch(i == 0?0:rand() %genome_count);
        }
        select();
    }

    // ============================================================
    // TARGETED BURST AFTER STAGNATION
    // ============================================================

    if(
        targetedStagnationLimit > 0 &&
        generationsWithoutImprovement >=
            targetedStagnationLimit
        )
    {
        printf(
            "\nTARGETED STAGNATION BURST "
            "generation=%d "
            "stagnation=%d\n",
            generation+1,
            generationsWithoutImprovement
            );

        fflush(stdout);

        /*
     * Apply intensive targeted search to the best individual.
     */

        targetedBestLocalSearch(
            0,
            targetedBurstIterations
            );

        /*
     * Optionally refine several other chromosomes.
     */

        int extra =
            min(
                localitems,
                genome_count
                );

        for(
            int i=1;
            i<extra;
            i++
            )
        {
            targetedBestLocalSearch(
                i,
                targetedBurstIterations/2
                );
        }

        select();

        previousBestFitness =
            fitness_array[0];

        generationsWithoutImprovement =
            0;
    }

    // --------------------------------------------------------
    // LOCAL CROSSOVER
    // --------------------------------------------------------

    for(int i=0;i<crossitems;i++)
    {
        int pos =(i == 0?0:rand() %genome_count);
        if(crossMethod =="targeted")
        {
            targetedCrossItem(pos,targetedCrossIterations);
        }
        else if(crossMethod =="targetedWorst")
        {
            targetedCrossWorstItem(pos,targetedCrossIterations);
        }
        else
        {
            // Original QGenClass method.
            crossItem(pos);
        }
    }

    select();

    crossover();

    if (generation)
        mutate();

    ++generation;
}

// ============================================================
// REPLACE WORST
// ============================================================

void Population::replaceWorst()
{
    vector<int> xtrial;

    xtrial.resize(
        genome_size
        );

    int randpos;

    randpos =
        rand() %
        genome_count;

    for (
        int i = 0;
        i < genome_size;
        i++
        )
    {
        double gamma;

        gamma =
            -0.5 +
            2.0 *
                rand() *
                1.0 /
                RAND_MAX;

        xtrial[i] =
            static_cast<int>(
                fabs(
                    (
                        1.0 +
                        gamma
                        ) *
                        genome[0][i] -
                    gamma *
                        genome[randpos][i]
                    )
                );
    }

    double ftrial =
        fitness(xtrial);

    if (
        fabs(ftrial) <
        fabs(
            fitness_array[
                genome_count -
                1
    ]
            )
        )
    {
        for (
            int i = 0;
            i < genome_size;
            i++
            )
        {
            genome[
                genome_count -
                1
            ][i] =
                xtrial[i];
        }

        fitness_array[
            genome_count -
            1
        ] =
            ftrial;
    }
}

// ============================================================
// MUTATION RATE
// ============================================================

void Population::setMutationRate(
    double r
    )
{
    if (
        r >= 0 &&
        r <= 1
        )
    {
        mutation_rate = r;
    }
}

// ============================================================
// SELECTION RATE
// ============================================================

void Population::setSelectionRate(
    double r
    )
{
    if (
        r >= 0 &&
        r <= 1
        )
    {
        selection_rate = r;
    }
}

double Population::getSelectionRate() const
{
    return selection_rate;
}

double Population::getMutationRate() const
{
    return mutation_rate;
}

// ============================================================
// BEST FITNESS
// ============================================================

double Population::getBestFitness() const
{
    return
        fitness_array[0];
}

// ============================================================
// BEST GENOME
// ============================================================

vector<int> Population::getBestGenome() const
{
    vector<int> g;

    g.resize(
        genome_size
        );

    for (
        int i = 0;
        i < genome_size;
        i++
        )
    {
        g[i] =
            genome[0][i];
    }

    return g;
}

// ============================================================
// DISCRETE GRADIENT
// ============================================================

vector<int> Population::discreteGradient(
    vector<int>& x
    )
{
    int n =
        static_cast<int>(
            x.size()
            );

    vector<int> grad(n);

    double fx =
        fitness(x);

    for (
        int i = 0;
        i < n;
        i++
        )
    {
        vector<int> x_plus =
            x;

        vector<int> x_minus =
            x;

        x_plus[i] +=
            1;

        x_minus[i] -=
            1;

        if (
            x_minus[i] <
            0
            )
        {
            x_minus[i] =
                0;
        }

        double f_plus =
            fitness(
                x_plus
                );

        double f_minus =
            fitness(
                x_minus
                );

        if (
            fabs(f_plus) <
            fabs(fx)
            )
        {
            grad[i] =
                +1;
        }
        else if (
            fabs(f_minus) <
            fabs(fx)
            )
        {
            grad[i] =
                -1;
        }
        else
        {
            grad[i] =
                0;
        }
    }

    return grad;
}

// ============================================================
// DISCRETE STEP
// ============================================================

vector<int> Population::discreteStep(
    vector<int>& x,
    vector<int>& grad
    )
{
    vector<int> res =
        x;

    for (
        int i = 0;
        i <
        static_cast<int>(
            x.size()
            );
        i++
        )
    {
        res[i] +=
            grad[i];
    }

    return res;
}

// ============================================================
// INTEGER LOCAL SEARCH
// ============================================================

void Population::integerLocalSearch(
    vector<int> &x,
    int maxSteps
    )
{
    double bestVal =
        fitness(x);

    int stepSize =
        100;

    for (
        int step = 0;
        step < maxSteps;
        step++
        )
    {
        vector<int> grad =
            discreteGradient(x);

        vector<int> candidate =
            x;

        for (
            int i = 0;
            i <
            static_cast<int>(
                x.size()
                );
            i++
            )
        {
            candidate[i] +=
                grad[i] *
                stepSize;

            if (
                candidate[i] <
                0
                )
            {
                candidate[i] =
                    0;
            }
        }

        double val =
            fitness(
                candidate
                );

        if (
            fabs(val) <
            fabs(bestVal)
            )
        {
            x =
                candidate;

            bestVal =
                val;
        }
        else
        {
            stepSize =
                max(
                    1,
                    stepSize /
                        2
                    );
        }

        printf(
            "GD[%4d]=%20.10lg\n",
            step,
            bestVal
            );
    }
}

// ============================================================
// INTEGER ADAM
// ============================================================

vector<int> Population::integerAdam(
    vector<int> x,
    int steps,
    double alpha,
    double beta1,
    double beta2,
    double eps
    )
{
    int n =
        static_cast<int>(
            x.size()
            );

    vector<double>
        m(
            n,
            0.0
            );

    vector<double>
        v(
            n,
            0.0
            );

    double bestVal =
        fitness(x);

    for (
        int t = 1;
        t <= steps;
        t++
        )
    {
        vector<int> g_int =
            discreteGradient(
                x
                );

        printf(
            "ADAM[%d] TRY: %20.10lf\n",
            t,
            bestVal
            );

        vector<double> g(n);

        for (
            int i = 0;
            i < n;
            i++
            )
        {
            g[i] =
                static_cast<double>(
                    g_int[i]
                    );
        }

        // ----------------------------------------------------
        // Moments
        // ----------------------------------------------------

        for (
            int i = 0;
            i < n;
            i++
            )
        {
            m[i] =
                beta1 *
                    m[i] +
                (
                    1 -
                    beta1
                    ) *
                    g[i];

            v[i] =
                beta2 *
                    v[i] +
                (
                    1 -
                    beta2
                    ) *
                    g[i] *
                    g[i];
        }

        // ----------------------------------------------------
        // Bias correction
        // ----------------------------------------------------

        vector<double>
            m_hat(n);

        vector<double>
            v_hat(n);

        for (
            int i = 0;
            i < n;
            i++
            )
        {
            m_hat[i] =
                m[i] /
                (
                    1 -
                    pow(
                        beta1,
                        t
                        )
                    );

            v_hat[i] =
                v[i] /
                (
                    1 -
                    pow(
                        beta2,
                        t
                        )
                    );
        }

        vector<int> candidate =
            x;

        for (
            int i = 0;
            i < n;
            i++
            )
        {
            double step =
                alpha *
                m_hat[i] /
                (
                    sqrt(
                        v_hat[i]
                        ) +
                    eps
                    );

            double p =
                fabs(step);

            if (
                rand() *
                    1.0 /
                    RAND_MAX <
                p
                )
            {
                candidate[i] +=
                    (
                        step > 0
                            ?
                            1
                            :
                            -1
                        );
            }

            if (
                candidate[i] <
                0
                )
            {
                candidate[i] =
                    0;
            }
        }

        double val =
            fitness(
                candidate
                );

        if (
            fabs(val) <
            fabs(bestVal)
            )
        {
            x =
                candidate;

            bestVal =
                val;
        }
        else
        {
            alpha *=
                0.7;
        }
    }

    return x;
}

// ============================================================
// NEW:
// CREATE CODON FOR TARGET PRODUCTION
//
// We require:
//
// newCodon % ruleCount == desiredRule
//
// The closest legal integer to oldCodon is selected.
// ============================================================

int Population::codonForRule(
    int oldCodon,
    int ruleCount,
    int desiredRule
    )
{
    if (ruleCount <= 0)
        return oldCodon;

    if (desiredRule < 0)
        return oldCodon;

    desiredRule %=
        ruleCount;

    if (oldCodon < 0)
        oldCodon = 0;

    int quotient =
        oldCodon /
        ruleCount;

    int candidate =
        quotient *
            ruleCount +
        desiredRule;

    int best =
        candidate;

    int bestDistance =
        abs(
            candidate -
            oldCodon
            );

    // --------------------------------------------------------
    // Candidate from previous modulo block
    // --------------------------------------------------------

    if (quotient > 0)
    {
        int lower =
            (
                quotient -
                1
                ) *
                ruleCount +
            desiredRule;

        if (lower >= 0)
        {
            int distance =
                abs(
                    lower -
                    oldCodon
                    );

            if (
                distance <
                bestDistance
                )
            {
                best =
                    lower;

                bestDistance =
                    distance;
            }
        }
    }

    // --------------------------------------------------------
    // Candidate from next modulo block
    // --------------------------------------------------------

    int upper =
        (
            quotient +
            1
            ) *
            ruleCount +
        desiredRule;

    if (upper >= 0)
    {
        int distance =
            abs(
                upper -
                oldCodon
                );

        if (
            distance <
            bestDistance
            )
        {
            best =
                upper;
        }
    }

    return best;
}

// ============================================================
// LOCAL METHOD
// ============================================================

void Population::setLocalMethod(
    string s
    )
{
    localMethod =
        s;
}

// ============================================================
// CROSS ONE ITEM
// ============================================================

void Population::crossItem(
    int pos
    )
{
    vector<int> g;

    g.resize(
        genome_size
        );

    for (
        int i = 0;
        i < genome_size;
        i++
        )
    {
        g[i] =
            genome[pos][i];
    }

    printf(
        "CROSS[%d]=",
        pos
        );

    fflush(stdout);

    for (
        int iters = 1;
        iters <= 100;
        iters++
        )
    {
        int gpos;
        int cutpoint;

    again:

        gpos =
            rand() %
            genome_count;

        cutpoint =
            rand() %
            genome_size;

        for (
            int j = 0;
            j < cutpoint;
            j++
            )
        {
            g[j] =
                genome[pos][j];
        }

        for (
            int j = cutpoint;
            j < genome_size;
            j++
            )
        {
            g[j] =
                genome[gpos][j];
        }

        double f =
            fitness(g);

        if (
            fabs(f) >
            1e+10
            )
        {
            goto again;
        }

        if (
            fabs(f) <
            fabs(
                fitness_array[pos]
                )
            )
        {
            printf(
                "%lf ",
                f
                );

            fflush(stdout);

            for (
                int j = 0;
                j < genome_size;
                j++
                )
            {
                genome[pos][j] =
                    g[j];
            }

            fitness_array[pos] =
                f;
        }
        else
        {
            for (
                int j = 0;
                j < cutpoint;
                j++
                )
            {
                g[j] =
                    genome[gpos][j];
            }

            for (
                int j = cutpoint;
                j < genome_size;
                j++
                )
            {
                g[j] =
                    genome[pos][j];
            }

            double f2 =
                fitness(g);

            if (
                fabs(f2) <
                fabs(
                    fitness_array[pos]
                    )
                )
            {
                printf(
                    "%lf ",
                    f2
                    );

                fflush(stdout);

                for (
                    int j = 0;
                    j < genome_size;
                    j++
                    )
                {
                    genome[pos][j] =
                        g[j];
                }

                fitness_array[pos] =
                    f2;
            }
        }
    }
}

// ============================================================
// RANDOM LOCAL MUTATION
// ============================================================

void Population::mutateItem(
    int pos
    )
{
    vector<int> g;

    g.resize(
        genome_size
        );

    for (
        int i = 0;
        i < genome_size;
        i++
        )
    {
        g[i] =
            genome[pos][i];
    }

    printf(
        "LOCAL[%d] = ",
        pos
        );

    fflush(stdout);

    for (
        int j = 0;
        j < 10;
        j++
        )
    {
        for (
            int i = 0;
            i < genome_size;
            i++
            )
        {
            int ik = 0;

            double f = 0.0;

            do
            {
                g[i] =
                    rand() %
                    MAX_RULE;

                ik++;

                if (ik == 10)
                    break;

                f =
                    fitness(g);

            }
            while (
                f <=
                fitness_array[pos]
                );

            if (ik != 10)
            {
                fitness_array[pos] =
                    f;

                printf(
                    " %lf ",
                    f
                    );

                fflush(stdout);

                genome[pos][i] =
                    g[i];
            }
            else
            {
                g[i] =
                    genome[pos][i];
            }
        }
    }

    printf("\n");
}

// ============================================================
// MUTATE ITEM AT CLASS
// ============================================================

void Population::mutateItemAtClass(
    int pos,
    int classIndex
    )
{
    vector<int> g;

    g.resize(
        genome_size
        );

    for (
        int i = 0;
        i < genome_size;
        i++
        )
    {
        g[i] =
            genome[pos][i];
    }

    printf(
        "LOCAL PERCLASS[%d] = ",
        pos
        );

    fflush(stdout);

    ClassProgram *p =
        (ClassProgram *)program;

    int partSize =
        genome_size /
        (
            p->getClass() -
            1
            );

    int start =
        classIndex *
        partSize;

    int end =
        (
            classIndex +
            1
            ) *
        partSize;

    if (
        end >=
        genome_size
        )
    {
        end =
            genome_size;
    }

    for (
        int j = 0;
        j < 10;
        j++
        )
    {
        for (
            int i = start;
            i < end;
            i++
            )
        {
            int ik = 0;

            double f = 0.0;

            do
            {
                g[i] =
                    rand() %
                    MAX_RULE;

                ik++;

                if (ik == 10)
                    break;

                f =
                    fitness(g);

            }
            while (
                f <=
                fitness_array[pos]
                );

            if (ik != 10)
            {
                fitness_array[pos] =
                    f;

                printf(
                    " %lf ",
                    f
                    );

                fflush(stdout);

                genome[pos][i] =
                    g[i];
            }
            else
            {
                g[i] =
                    genome[pos][i];
            }
        }
    }

    printf("\n");
}

// ============================================================
// NEW:
// TARGETED GENOTYPE LOCAL SEARCH
//
// This operates directly on the chromosome.
//
// The ClassProgram mapper tells us which codon produced:
//
// FUNCTION
// BINARY_OPERATOR
// BOOLEAN_OPERATOR
// VARIABLE
//
// For a selected semantic codon, every alternative production
// of the same non-terminal is evaluated.
//
// Only an improvement is accepted.
// ============================================================

// ============================================================
// TARGETED WORST-CLASS LOCAL SEARCH
//
// 1. Find the class with the largest classification error.
// 2. Identify the chromosome segment corresponding to it.
// 3. Obtain the active semantic codons using CodonTrace.
// 4. Keep only codons belonging to the worst-class segment.
// 5. Try semantic alternatives:
//       cos -> sin
//       +   -> *
//       <   -> >=
//       x2  -> x7
// 6. Accept only fitness improvements.
//
// IMPORTANT:
//
// QGenClass uses nclass-1 explicit rules.
// The final class is the default ELSE class.
//
// Therefore:
//
// worstClass < nclass-1
//      -> search only its chromosome segment.
//
// worstClass == nclass-1
//      -> there is no explicit segment.
//         Search all active semantic codons.
// ============================================================

void Population::targetedWorstLocalSearch(
    int pos,
    int iterations
    )
{
    // --------------------------------------------------------
    // Basic checks
    // --------------------------------------------------------

    if(
        pos < 0 ||
        pos >= genome_count
        )
    {
        return;
    }

    if(iterations <= 0)
        return;

    if(genome_size <= 0)
        return;

    ClassProgram *p =
        (ClassProgram *)program;

    if(p == NULL)
        return;

    const int classCount =
        p->getClass();

    if(classCount <= 1)
        return;

    // --------------------------------------------------------
    // Copy chromosome
    // --------------------------------------------------------

    vector<int> current(
        genome_size
        );

    for(
        int i=0;
        i<genome_size;
        i++
        )
    {
        current[i] =
            genome[pos][i];
    }

    double bestFitness =
        fitness(current);

    // ========================================================
    // LOCAL SEARCH
    // ========================================================

    int acceptedMoves = 0;

    for(
        int iteration=0;
        iteration<iterations;
        iteration++
        )
    {
        // ====================================================
        // 1. FIND CURRENT WORST CLASS
        //
        // Recalculate after every accepted mutation because
        // the identity of the worst class may change.
        // ====================================================

        vector<double> classError;

        p->getErrorPerClass(
            current,
            classError
            );

        if(classError.empty())
            break;

        int worstClass = -1;

        double worstError =
            -1.0;

        for(
            int c=0;
            c<(int)classError.size();
            c++
            )
        {
            if(
                classError[c] >
                worstError
                )
            {
                worstError =
                    classError[c];

                worstClass =
                    c;
            }
        }

        if(worstClass < 0)
            break;

        // ====================================================
        // 2. GET ACTIVE CODON TRACE
        // ====================================================

        vector<CodonTrace> fullTrace;

        p->getCodonTrace(
            current,
            fullTrace
            );

        if(fullTrace.empty())
            break;

        // ====================================================
        // 3. DETERMINE WORST CLASS SEGMENT
        // ====================================================

        const int explicitRules =
            classCount - 1;

        const int partSize =
            genome_size /
            explicitRules;

        int segmentStart = 0;
        int segmentEnd =
            genome_size;

        bool useWholeGenome =
            false;

        /*
         * The last class is the default ELSE class and has
         * no explicit chromosome segment.
         */

        if(
            worstClass >=
            explicitRules
            )
        {
            useWholeGenome =
                true;
        }
        else
        {
            segmentStart =
                worstClass *
                partSize;

            segmentEnd =
                (
                    worstClass + 1
                    ) *
                partSize;

            /*
             * Include any remainder in the final explicit
             * segment if necessary.
             */
            if(
                worstClass ==
                explicitRules - 1
                )
            {
                segmentEnd =
                    genome_size;
            }
        }

        // ====================================================
        // 4. KEEP ONLY TRACE ENTRIES THAT BELONG TO THE
        //    WORST CLASS
        // ====================================================

        vector<CodonTrace>
            candidates;

        for(
            const CodonTrace &item :
            fullTrace
            )
        {
            if(
                item.genomePos < 0 ||
                item.genomePos >=
                    genome_size
                )
            {
                continue;
            }

            if(useWholeGenome)
            {
                candidates.push_back(
                    item
                    );
            }
            else
            {
                if(
                    item.genomePos >=
                        segmentStart &&
                    item.genomePos <
                        segmentEnd
                    )
                {
                    candidates.push_back(
                        item
                        );
                }
            }
        }

        if(candidates.empty())
            continue;

        // ====================================================
        // 5. CHOOSE ONE ACTIVE SEMANTIC CODON
        // ====================================================

        const int traceIndex =
            rand() %
            static_cast<int>(
                candidates.size()
                );

        const CodonTrace item =
            candidates[
                traceIndex
        ];

        if(item.ruleCount <= 1)
            continue;

        if(
            item.selectedRule < 0 ||
            item.selectedRule >=
                item.ruleCount
            )
        {
            continue;
        }

        const int originalCodon =
            current[
                item.genomePos
        ];

        // ====================================================
        // 6. TRY ALL SEMANTIC ALTERNATIVES
        // ====================================================

        vector<int> bestCandidate =
            current;

        double localBestFitness =
            bestFitness;

        int bestProduction =
            item.selectedRule;

        for(
            int desiredRule=0;
            desiredRule<
            item.ruleCount;
            desiredRule++
            )
        {
            if(
                desiredRule ==
                item.selectedRule
                )
            {
                continue;
            }

            vector<int> trial =
                current;

            int newCodon =
                codonForRule(
                    originalCodon,
                    item.ruleCount,
                    desiredRule
                    );

            if(
                newCodon ==
                originalCodon
                )
            {
                continue;
            }

            trial[
                item.genomePos
            ] =
                newCodon;

            double trialFitness =
                fitness(trial);

            /*
             * Population::fitness() is maximized.
             */
            if(
                trialFitness >
                localBestFitness
                )
            {
                localBestFitness =
                    trialFitness;

                bestCandidate =
                    trial;

                bestProduction =
                    desiredRule;
            }
        }

        // ====================================================
        // 7. ACCEPT IMPROVEMENT
        // ====================================================

        if(
            localBestFitness >
            bestFitness
            )
        {
            const char *typeName =
                "UNKNOWN";

            if(
                item.type ==
                CodonType::FUNCTION
                )
            {
                typeName =
                    "FUNCTION";
            }
            else if(
                item.type ==
                CodonType::
                BINARY_OPERATOR
                )
            {
                typeName =
                    "BINARY_OPERATOR";
            }
            else if(
                item.type ==
                CodonType::
                BOOLEAN_OPERATOR
                )
            {
                typeName =
                    "BOOLEAN_OPERATOR";
            }
            else if(
                item.type ==
                CodonType::VARIABLE
                )
            {
                typeName =
                    "VARIABLE";
            }

            const int newCodon =
                bestCandidate[
                    item.genomePos
            ];

            printf(
                "TARGETED_WORST[%d] "
                "iter=%d "
                "worstClass=%d "
                "error=%.6lf "
                "type=%s "
                "codonPos=%d "
                "codon=%d->%d "
                "rule=%d->%d "
                "fitness=%.10lf->%.10lf\n",
                pos,
                iteration + 1,
                worstClass,
                worstError,
                typeName,
                item.genomePos,
                originalCodon,
                newCodon,
                item.selectedRule,
                bestProduction,
                bestFitness,
                localBestFitness
                );

            fflush(stdout);

            current =
                bestCandidate;

            bestFitness =
                localBestFitness;

            acceptedMoves++;
        }
    }

    // ========================================================
    // 8. COPY IMPROVED CHROMOSOME BACK
    // ========================================================

    for(
        int i=0;
        i<genome_size;
        i++
        )
    {
        genome[pos][i] =
            current[i];
    }

    fitness_array[pos] =
        bestFitness;

    printf(
        "TARGETED_WORST[%d] "
        "END fitness=%.10lf "
        "accepted=%d\n",
        pos,
        bestFitness,
        acceptedMoves
        );

    fflush(stdout);
}
void Population::targetedLocalSearch(
    int pos,
    int iterations
    )
{
    // --------------------------------------------------------
    // Validate arguments
    // --------------------------------------------------------

    if (
        pos < 0 ||
        pos >= genome_count
        )
    {
        return;
    }

    if (iterations <= 0)
        return;

    if (genome_size <= 0)
        return;

    // --------------------------------------------------------
    // Targeted search currently requires ClassProgram.
    // --------------------------------------------------------

    ClassProgram *p =
        (ClassProgram *)program;

    if (p == NULL)
        return;

    // --------------------------------------------------------
    // Copy current genome
    // --------------------------------------------------------

    vector<int> current;

    current.resize(
        genome_size
        );

    for (
        int i = 0;
        i < genome_size;
        i++
        )
    {
        current[i] =
            genome[pos][i];
    }

    /*
     * IMPORTANT:
     *
     * Population::fitness() returns:
     *
     *     -program->fitness()
     *
     * Therefore larger values are better.
     */

    double bestFitness =
        fitness(current);

    printf(
        "TARGETED[%d] START fitness=%20.10lf\n",
        pos,
        bestFitness
        );

    fflush(stdout);

    int acceptedMoves =
        0;

    // ========================================================
    // TARGETED ITERATIONS
    // ========================================================

    for (
        int iteration = 0;
        iteration < iterations;
        iteration++
        )
    {
        // ----------------------------------------------------
        // Recompute mapping trace.
        //
        // This must happen after every accepted change because
        // a change in an early codon can alter the subsequent
        // phenotype and therefore the active codons.
        // ----------------------------------------------------

        vector<CodonTrace> trace;

        p->getCodonTrace(
            current,
            trace
            );

        if (trace.empty())
        {
            if (iteration == 0)
            {
                printf(
                    "TARGETED[%d] no trace entries found\n",
                    pos
                    );

                fflush(stdout);
            }

            break;
        }

        // ----------------------------------------------------
        // Randomly select one meaningful grammar decision.
        // ----------------------------------------------------

        int traceIndex =
            rand() %
            static_cast<int>(
                trace.size()
                );

        CodonTrace item =
            trace[
                static_cast<size_t>(
                    traceIndex
                    )
        ];

        if (
            item.genomePos < 0 ||
            item.genomePos >= genome_size
            )
        {
            continue;
        }

        if (
            item.ruleCount <=
            1
            )
        {
            continue;
        }

        if (
            item.selectedRule <
                0 ||
            item.selectedRule >=
                item.ruleCount
            )
        {
            continue;
        }

        // ----------------------------------------------------
        // Best candidate for this grammar decision.
        // ----------------------------------------------------

        vector<int> bestCandidate =
            current;

        double localBestFitness =
            bestFitness;

        int bestProduction =
            item.selectedRule;

        int originalCodon =
            current[
                static_cast<size_t>(
                    item.genomePos
                    )
        ];

        // ====================================================
        // EXHAUSTIVE SEMANTIC NEIGHBORHOOD
        //
        // Example:
        //
        // FUNCTION:
        //
        // current production = cos
        //
        // Try:
        //
        // cos -> sin
        // cos -> exp
        // cos -> log
        //
        // depending on the actual rule ordering in grammar.
        // ====================================================

        for (
            int desiredRule = 0;
            desiredRule <
            item.ruleCount;
            desiredRule++
            )
        {
            if (
                desiredRule ==
                item.selectedRule
                )
            {
                continue;
            }

            vector<int> candidate =
                current;

            int newCodon =
                codonForRule(
                    originalCodon,
                    item.ruleCount,
                    desiredRule
                    );

            if (
                newCodon ==
                originalCodon
                )
            {
                continue;
            }

            candidate[
                static_cast<size_t>(
                    item.genomePos
                    )
            ] =
                newCodon;

            double candidateFitness =
                fitness(
                    candidate
                    );

            // ------------------------------------------------
            // Population fitness is maximized.
            // ------------------------------------------------

            if (
                candidateFitness >
                localBestFitness
                )
            {
                localBestFitness =
                    candidateFitness;

                bestCandidate =
                    candidate;

                bestProduction =
                    desiredRule;
            }
        }

        // ====================================================
        // ACCEPT BEST IMPROVING PRODUCTION
        // ====================================================

        if (
            localBestFitness >
            bestFitness
            )
        {
            const char *typeName =
                "UNKNOWN";

            if (
                item.type ==
                CodonType::FUNCTION
                )
            {
                typeName =
                    "FUNCTION";
            }
            else if (
                item.type ==
                CodonType::
                BINARY_OPERATOR
                )
            {
                typeName =
                    "BINARY_OPERATOR";
            }
            else if (
                item.type ==
                CodonType::
                BOOLEAN_OPERATOR
                )
            {
                typeName =
                    "BOOLEAN_OPERATOR";
            }
            else if (
                item.type ==
                CodonType::VARIABLE
                )
            {
                typeName =
                    "VARIABLE";
            }
            else if (
                item.type ==
                CodonType::
                CLASS_OUTPUT
                )
            {
                typeName =
                    "CLASS_OUTPUT";
            }

            int newCodon =
                bestCandidate[
                    static_cast<size_t>(
                        item.genomePos
                        )
            ];

            printf(
                "TARGETED[%d] "
                "iter=%d "
                "type=%s "
                "codonPos=%d "
                "codon=%d->%d "
                "rule=%d->%d "
                "fitness=%20.10lf->%20.10lf\n",
                pos,
                iteration + 1,
                typeName,
                item.genomePos,
                originalCodon,
                newCodon,
                item.selectedRule,
                bestProduction,
                bestFitness,
                localBestFitness
                );

            fflush(stdout);

            current =
                bestCandidate;

            bestFitness =
                localBestFitness;

            acceptedMoves++;
        }
    }

    // ========================================================
    // COPY IMPROVED GENOME BACK
    // ========================================================

    for (
        int i = 0;
        i < genome_size;
        i++
        )
    {
        genome[pos][i] =
            current[
                static_cast<size_t>(
                    i
                    )
        ];
    }

    fitness_array[pos] =
        bestFitness;

    printf(
        "TARGETED[%d] END fitness=%20.10lf accepted=%d\n",
        pos,
        bestFitness,
        acceptedMoves
        );

    fflush(stdout);
}

// ============================================================
// LOCAL SEARCH DISPATCHER
// ============================================================

void Population::localSearch(
    int pos
    )
{
    vector<int> g;

    g.resize(
        genome_size
        );

    for (
        int i = 0;
        i < genome_size;
        i++
        )
    {
        g[i] =
            genome[pos][i];
    }

    // ========================================================
    // CROSSOVER LOCAL SEARCH
    // ========================================================

    if (
        localMethod ==
        "crossover"
        )
    {
        crossItem(pos);
    }

    // ========================================================
    // MUTATE WORST CLASS
    // ========================================================

    else if (
        localMethod ==
        "mutateWorst"
        )
    {
        ClassProgram *p =
            (ClassProgram *)program;

        vector<double> val;

        val.resize(
            p->getClass()
            );

        p->getErrorPerClass(
            g,
            val
            );

        int maxIndex =
            0;

        double maxValue =
            val[0];

        for (
            int i = 0;
            i <
            static_cast<int>(
                val.size()
                );
            i++
            )
        {
            if (
                val[i] >
                maxValue
                )
            {
                maxIndex =
                    i;

                maxValue =
                    val[i];
            }
        }

        printf(
            "MUTATE %d Index with Value %lf\n",
            maxIndex,
            maxValue
            );

        if (
            maxIndex ==
            p->getClass() -
                1
            )
        {
            mutateItem(pos);
        }
        else
        {
            mutateItemAtClass(
                pos,
                maxIndex
                );
        }
    }

    // ========================================================
    // RANDOM LOCAL MUTATION
    // ========================================================

    else if (
        localMethod ==
        "mutate"
        )
    {
        mutateItem(pos);
    }

    // ========================================================
    // SIMULATED ANNEALING
    // ========================================================

    else if (
        localMethod ==
        "siman"
        )
    {
        double f =
            fitness_array[pos];

        g =
            simulatedAnnealing(
                g
                );

        fitness_array[pos] =
            fitness(g);

        for (
            int j = 0;
            j < genome_size;
            j++
            )
        {
            genome[pos][j] =
                g[j];
        }

        printf(
            "SIMAN[%d] %lf=>%lf\n",
            pos,
            f,
            fitness_array[pos]
            );
    }

    // ========================================================
    // DIFFERENTIAL EVOLUTION LOCAL SEARCH
    // ========================================================

    else if (
        localMethod ==
        "de"
        )
    {
        int randomA;
        int randomB;
        int randomC;

        do
        {
            randomA =
                rand() %
                genome_count;

            randomB =
                rand() %
                genome_count;

            randomC =
                rand() %
                genome_count;

        }
        while (
            randomA ==
                randomB ||
            randomB ==
                randomC ||
            randomC ==
                randomA
            );

        double CR =
            0.9;

        double F =
            0.8;

        int randomIndex =
            rand() %
            genome_size;

        for (
            int i = 0;
            i < genome_size;
            i++
            )
        {
            if (
                i ==
                    randomIndex ||
                rand() *
                        1.0 /
                        RAND_MAX <=
                    CR
                )
            {
                int old_value =
                    genome[pos][i];

                F =
                    -0.5 +
                    2.0 *
                        rand() *
                        1.0 /
                        RAND_MAX;

                genome[pos][i] =
                    genome[
                        randomA
                ][i] +
                    abs(
                        F *
                        (
                            genome[
                                randomB
                ][i] -
                            genome[
                                randomC
                ][i]
                            )
                        );

                if (
                    genome[pos][i] <
                    0
                    )
                {
                    genome[pos][i] =
                        old_value;

                    continue;
                }

                for (
                    int j = 0;
                    j < genome_size;
                    j++
                    )
                {
                    g[j] =
                        genome[pos][j];
                }

                double trial_fitness =
                    fitness(g);

                if (
                    fabs(
                        trial_fitness
                        ) <
                    fabs(
                        fitness_array[
                            pos
                ]
                        )
                    )
                {
                    fitness_array[pos] =
                        trial_fitness;
                }
                else
                {
                    genome[pos][i] =
                        old_value;
                }
            }
        }
    }

    // ========================================================
    // INTEGER GRADIENT DESCENT
    // ========================================================

    else if (
        localMethod ==
        "gd"
        )
    {
        integerLocalSearch(
            g
            );

        double ff =
            fitness(g);

        if (
            fabs(ff) <
            fabs(
                fitness_array[
                    pos
        ]
                )
            )
        {
            printf(
                "GD. NEW VALUE[%d] = %lf=>%lf\n",
                pos,
                fitness_array[pos],
                ff
                );

            fitness_array[pos] =
                ff;

            for (
                int j = 0;
                j <
                static_cast<int>(
                    g.size()
                    );
                j++
                )
            {
                genome[pos][j] =
                    g[j];
            }
        }
    }

    // ========================================================
    // INTEGER ADAM
    // ========================================================

    else if (
        localMethod ==
        "adam"
        )
    {
        g =
            integerAdam(
                g
                );

        double ff =
            fitness(g);

        if (
            fabs(ff) <
            fabs(
                fitness_array[
                    pos
        ]
                )
            )
        {
            printf(
                "ADAM. NEW VALUE[%d] = %lf=>%lf\n",
                pos,
                fitness_array[pos],
                ff
                );

            fitness_array[pos] =
                ff;

            for (
                int j = 0;
                j <
                static_cast<int>(
                    g.size()
                    );
                j++
                )
            {
                genome[pos][j] =
                    g[j];
            }
        }
    }

    // ========================================================
    // NEW:
    // TARGETED GENOTYPE LOCAL SEARCH
    // ========================================================

    else if (
        localMethod ==
        "targeted"
        )
    {
        targetedLocalSearch(
            pos,
            targetedIterations
            );
    }
    else if(
        localMethod ==
        "targetedWorst"
        )
    {
        targetedWorstLocalSearch(
            pos,
            targetedIterations
            );
    }
    else if(
        localMethod ==
        "targetedBest"
        )
    {
          printf("BEST!!!!!!!!\n");
        targetedBestLocalSearch(
            pos,
            targetedIterations
            );
    }
    else if(
        localMethod ==
        "constants"
        )
    {
        printf("CONST!!!!!!!!\n");
        constantsLocalSearch(
            pos,
            targetedIterations
            );
    }
}

// ============================================================
// EVALUATE BEST FITNESS
// ============================================================

double Population::evaluateBestFitness()
{
    vector<int> g;

    g.resize(
        genome_size
        );

    for (
        int i = 0;
        i < genome_size;
        i++
        )
    {
        g[i] =
            genome[0][i];
    }

    return
        fitness(g);
}
void Population::setTargetedStagnationLimit(
    int n
    )
{
    if(n >= 0)
        targetedStagnationLimit = n;
}

void Population::setTargetedBurstIterations(
    int n
    )
{
    if(n > 0)
        targetedBurstIterations = n;
}
void Population::printTargetedStatistics() const
{
    printf("\n");
    printf("============================================================\n");
    printf(" TARGETED MUTATION STATISTICS\n");
    printf("============================================================\n");

    printf(
        "FUNCTION  attempts=%ld accepted=%ld rate=%.6lf\n",
        targetedFunctionAttempts,
        targetedFunctionAccepted,
        targetedFunctionAttempts > 0
            ?
            targetedFunctionAccepted * 1.0 /
                targetedFunctionAttempts
            :
            0.0
        );

    printf(
        "BINARY    attempts=%ld accepted=%ld rate=%.6lf\n",
        targetedBinaryAttempts,
        targetedBinaryAccepted,
        targetedBinaryAttempts > 0
            ?
            targetedBinaryAccepted * 1.0 /
                targetedBinaryAttempts
            :
            0.0
        );

    printf(
        "BOOLEAN   attempts=%ld accepted=%ld rate=%.6lf\n",
        targetedBooleanAttempts,
        targetedBooleanAccepted,
        targetedBooleanAttempts > 0
            ?
            targetedBooleanAccepted * 1.0 /
                targetedBooleanAttempts
            :
            0.0
        );

    printf(
        "VARIABLE  attempts=%ld accepted=%ld rate=%.6lf\n",
        targetedVariableAttempts,
        targetedVariableAccepted,
        targetedVariableAttempts > 0
            ?
            targetedVariableAccepted * 1.0 /
                targetedVariableAttempts
            :
            0.0
        );

    printf("============================================================\n");
}
// ============================================================
// DESTRUCTOR
// ============================================================


void Population::targetedBestLocalSearch(
    int pos,
    int iterations
    )
{
    if(
        pos < 0 ||
        pos >= genome_count
        )
        return;

    if(iterations <= 0)
        return;

    ClassProgram *p =
        (ClassProgram *)program;

    if(p == NULL)
        return;

    vector<int> current(
        genome_size
        );

    for(
        int i=0;
        i<genome_size;
        i++
        )
    {
        current[i] =
            genome[pos][i];
    }

    double bestFitness =
        fitness(current);

    printf(
        "TARGETED_BEST[%d] START fitness=%.10lf\n",
        pos,
        bestFitness
        );

    fflush(stdout);

    int acceptedMoves = 0;

    // ========================================================
    // LOCAL SEARCH ITERATIONS
    // ========================================================

    for(
        int iteration=0;
        iteration<iterations;
        iteration++
        )
    {
        vector<CodonTrace> trace;

        p->getCodonTrace(
            current,
            trace
            );

        if(trace.empty())
            break;

        // ====================================================
        // ADAPTIVE SUCCESS RATES
        // ====================================================

        double functionRate =
            targetedSuccessRate(
                targetedFunctionAttempts,
                targetedFunctionAccepted
                );

        double binaryRate =
            targetedSuccessRate(
                targetedBinaryAttempts,
                targetedBinaryAccepted
                );

        double booleanRate =
            targetedSuccessRate(
                targetedBooleanAttempts,
                targetedBooleanAccepted
                );

        double variableRate =
            targetedSuccessRate(
                targetedVariableAttempts,
                targetedVariableAccepted
                );

        // ====================================================
        // FIND GLOBAL BEST MOVE
        // ====================================================

        vector<int> globalBestGenome =
            current;

        double globalBestFitness =
            bestFitness;

        int globalCodonPos =
            -1;

        int globalOldRule =
            -1;

        int globalNewRule =
            -1;

        CodonType globalType =
            CodonType::UNKNOWN;

        // ====================================================
        // TEST ALL ACTIVE SEMANTIC CODONS
        // ====================================================

        for(
            size_t t=0;
            t<trace.size();
            t++
            )
        {
            const CodonTrace &item =
                trace[t];

            if(
                item.genomePos < 0 ||
                item.genomePos >= genome_size
                )
                continue;

            if(item.ruleCount <= 1)
                continue;

            // ------------------------------------------------
            // Adaptive sampling
            // ------------------------------------------------

            double probability =
                1.0;

            if(
                item.type ==
                CodonType::FUNCTION
                )
            {
                probability =
                    functionRate;
            }
            else if(
                item.type ==
                CodonType::
                BINARY_OPERATOR
                )
            {
                probability =
                    binaryRate;
            }
            else if(
                item.type ==
                CodonType::
                BOOLEAN_OPERATOR
                )
            {
                probability =
                    booleanRate;
            }
            else if(
                item.type ==
                CodonType::VARIABLE
                )
            {
                probability =
                    variableRate;
            }

            /*
             * Keep a minimum exploration probability.
             */
            probability =
                0.20 +
                0.80 *
                    probability;

            double r =
                rand() *
                1.0 /
                RAND_MAX;

            if(r > probability)
                continue;

            int originalCodon =
                current[
                    item.genomePos
            ];

            // =================================================
            // TRY ALL PRODUCTIONS FOR THIS CODON
            // =================================================

            for(
                int desiredRule=0;
                desiredRule<
                item.ruleCount;
                desiredRule++
                )
            {
                if(
                    desiredRule ==
                    item.selectedRule
                    )
                    continue;

                // --------------------------------------------
                // Statistics
                // --------------------------------------------

                if(
                    item.type ==
                    CodonType::FUNCTION
                    )
                    targetedFunctionAttempts++;

                else if(
                    item.type ==
                    CodonType::
                    BINARY_OPERATOR
                    )
                    targetedBinaryAttempts++;

                else if(
                    item.type ==
                    CodonType::
                    BOOLEAN_OPERATOR
                    )
                    targetedBooleanAttempts++;

                else if(
                    item.type ==
                    CodonType::VARIABLE
                    )
                    targetedVariableAttempts++;

                vector<int> candidate =
                    current;

                candidate[
                    item.genomePos
                ] =
                    codonForRule(
                        originalCodon,
                        item.ruleCount,
                        desiredRule
                        );

                double candidateFitness =
                    fitness(candidate);

                // =============================================
                // GLOBAL BEST IMPROVEMENT
                // =============================================

                if(
                    candidateFitness >
                    globalBestFitness
                    )
                {
                    globalBestFitness =
                        candidateFitness;

                    globalBestGenome =
                        candidate;

                    globalCodonPos =
                        item.genomePos;

                    globalOldRule =
                        item.selectedRule;

                    globalNewRule =
                        desiredRule;

                    globalType =
                        item.type;
                }
            }
        }

        // ====================================================
        // NO IMPROVEMENT
        // ====================================================

        if(
            globalBestFitness <=
            bestFitness
            )
        {
            /*
             * We reached a local optimum for the currently
             * examined semantic neighborhood.
             */
            break;
        }

        // ====================================================
        // ACCEPT GLOBAL BEST MOVE
        // ====================================================

        const char *typeName =
            "UNKNOWN";

        if(
            globalType ==
            CodonType::FUNCTION
            )
        {
            typeName =
                "FUNCTION";

            targetedFunctionAccepted++;
        }
        else if(
            globalType ==
            CodonType::
            BINARY_OPERATOR
            )
        {
            typeName =
                "BINARY_OPERATOR";

            targetedBinaryAccepted++;
        }
        else if(
            globalType ==
            CodonType::
            BOOLEAN_OPERATOR
            )
        {
            typeName =
                "BOOLEAN_OPERATOR";

            targetedBooleanAccepted++;
        }
        else if(
            globalType ==
            CodonType::VARIABLE
            )
        {
            typeName =
                "VARIABLE";

            targetedVariableAccepted++;
        }

        printf(
            "TARGETED_BEST[%d] "
            "iter=%d "
            "type=%s "
            "codon=%d "
            "rule=%d->%d "
            "fitness=%.10lf->%.10lf\n",
            pos,
            iteration+1,
            typeName,
            globalCodonPos,
            globalOldRule,
            globalNewRule,
            bestFitness,
            globalBestFitness
            );

        fflush(stdout);

        current =
            globalBestGenome;

        bestFitness =
            globalBestFitness;

        acceptedMoves++;
    }

    // ========================================================
    // COPY BACK
    // ========================================================

    for(
        int i=0;
        i<genome_size;
        i++
        )
    {
        genome[pos][i] =
            current[i];
    }

    fitness_array[pos] =
        bestFitness;

    printf(
        "TARGETED_BEST[%d] END fitness=%.10lf accepted=%d\n",
        pos,
        bestFitness,
        acceptedMoves
        );

    fflush(stdout);
}
// ============================================================
// NUMERICAL CONSTANTS LOCAL SEARCH
//
// This local optimizer changes ONLY codons involved in the
// generation of numerical constants.
//
// It does NOT modify:
//
//   FUNCTION
//   VARIABLE
//   BINARY_OPERATOR
//   BOOLEAN_OPERATOR
//
// Therefore the symbolic structure of the expression is
// preserved as much as possible.
//
// Typical modifications:
//
//     12.45 -> 17.45
//     12.45 -> 12.95
//     12.45 -> -12.45
//
// depending on the grammar productions.
// ============================================================

void Population::constantsLocalSearch(
    int pos,
    int iterations
    )
{
    // --------------------------------------------------------
    // Checks
    // --------------------------------------------------------

    if(
        pos < 0 ||
        pos >= genome_count
        )
        return;

    if(iterations <= 0)
        return;

    ClassProgram *p =
        (ClassProgram *)program;

    if(p == NULL)
        return;

    // --------------------------------------------------------
    // Copy current chromosome
    // --------------------------------------------------------

    vector<int> current(
        genome_size
        );

    for(
        int i=0;
        i<genome_size;
        i++
        )
    {
        current[i] =
            genome[pos][i];
    }

    /*
     * Population::fitness() is maximized because it returns:
     *
     *      -program->fitness(...)
     */
    double bestFitness =
        fitness(current);

    printf(
        "CONSTANTS[%d] START fitness=%.10lf\n",
        pos,
        bestFitness
        );

    fflush(stdout);

    int acceptedMoves = 0;

    // ========================================================
    // LOCAL SEARCH ITERATIONS
    // ========================================================

    for(
        int iteration=0;
        iteration<iterations;
        iteration++
        )
    {
        // ----------------------------------------------------
        // Rebuild trace because an accepted constant change
        // can affect subsequent active codons.
        // ----------------------------------------------------

        vector<CodonTrace> trace;

        p->getCodonTrace(
            current,
            trace
            );

        // ----------------------------------------------------
        // Keep only constant-related codons
        // ----------------------------------------------------

        vector<CodonTrace>
            constantTrace;

        for(
            const CodonTrace &item :
            trace
            )
        {
            if(
                item.type ==
                CodonType::CONSTANT
                )
            {
                constantTrace.push_back(
                    item
                    );
            }
        }

        if(constantTrace.empty())
        {
            if(iteration == 0)
            {
                printf(
                    "CONSTANTS[%d] "
                    "no numerical constants found\n",
                    pos
                    );

                fflush(stdout);
            }

            break;
        }

        // ----------------------------------------------------
        // Select one active constant-related codon
        // ----------------------------------------------------

        int selected =
            rand() %
            static_cast<int>(
                constantTrace.size()
                );

        CodonTrace item =
            constantTrace[
                static_cast<size_t>(
                    selected
                    )
        ];

        if(
            item.genomePos < 0 ||
            item.genomePos >=
                genome_size
            )
        {
            continue;
        }

        if(item.ruleCount <= 1)
            continue;

        if(
            item.selectedRule < 0 ||
            item.selectedRule >=
                item.ruleCount
            )
        {
            continue;
        }

        // ----------------------------------------------------
        // Current integer codon
        // ----------------------------------------------------

        int originalCodon =
            current[
                static_cast<size_t>(
                    item.genomePos
                    )
        ];

        vector<int> bestCandidate =
            current;

        double localBestFitness =
            bestFitness;

        int bestProduction =
            item.selectedRule;

        // ====================================================
        // TRY ALL ALTERNATIVE PRODUCTIONS
        //
        // Only alternatives of this constant-related
        // non-terminal are examined.
        // ====================================================

        for(
            int desiredRule=0;
            desiredRule<
            item.ruleCount;
            desiredRule++
            )
        {
            if(
                desiredRule ==
                item.selectedRule
                )
            {
                continue;
            }

            vector<int> candidate =
                current;

            int newCodon =
                codonForRule(
                    originalCodon,
                    item.ruleCount,
                    desiredRule
                    );

            if(newCodon == originalCodon)
                continue;

            candidate[
                static_cast<size_t>(
                    item.genomePos
                    )
            ] =
                newCodon;

            double candidateFitness =
                fitness(candidate);

            // ------------------------------------------------
            // Larger Population fitness is better.
            // ------------------------------------------------

            if(
                candidateFitness >
                localBestFitness
                )
            {
                localBestFitness =
                    candidateFitness;

                bestCandidate =
                    candidate;

                bestProduction =
                    desiredRule;
            }
        }

        // ====================================================
        // ACCEPT
        // ====================================================

        if(
            localBestFitness >
            bestFitness
            )
        {
            int newCodon =
                bestCandidate[
                    static_cast<size_t>(
                        item.genomePos
                        )
            ];

            printf(
                "CONSTANTS[%d] "
                "iter=%d "
                "codonPos=%d "
                "codon=%d->%d "
                "rule=%d->%d "
                "fitness=%.10lf->%.10lf\n",
                pos,
                iteration + 1,
                item.genomePos,
                originalCodon,
                newCodon,
                item.selectedRule,
                bestProduction,
                bestFitness,
                localBestFitness
                );

            fflush(stdout);

            current =
                bestCandidate;

            bestFitness =
                localBestFitness;

            acceptedMoves++;
        }
    }

    // ========================================================
    // COPY BEST CHROMOSOME BACK
    // ========================================================

    for(
        int i=0;
        i<genome_size;
        i++
        )
    {
        genome[pos][i] =
            current[
                static_cast<size_t>(
                    i
                    )
        ];
    }

    fitness_array[pos] =
        bestFitness;

    printf(
        "CONSTANTS[%d] "
        "END fitness=%.10lf "
        "accepted=%d\n",
        pos,
        bestFitness,
        acceptedMoves
        );

    fflush(stdout);
}
Population::~Population()
{
    for (
        int i = 0;
        i < genome_count;
        i++
        )
    {
        delete[]
            children[i];

        delete[]
            genome[i];
    }

    delete[]
        genome;

    delete[]
        children;

    delete[]
        fitness_array;
}
