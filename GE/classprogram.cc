#include <GE/classprogram.h>

#include <math.h>

#define NAN_CLASS 1e+10

#include <iostream>
using std::cerr;
using std::endl;

#include <fstream>
using std::ofstream;

#include <string>
#include <cstdlib>
#include <regex>
#include <iterator>
#include <vector>

// *******************************************************************
#pragma GCC optimize("unroll-loops","omit-frame-pointer","inline", "unsafe-math-optimizations")
#pragma GCC option("arch=native","tune=native","no-zero-upper")
// *******************************************************************

static double dmax(double a,double b)
{
    return a>b?a:b;
}

int problem_dimension;

extern int wrapping;

// ============================================================
// CONSTRUCTOR
// ============================================================

ClassProgram::ClassProgram(Dataset *tr,Dataset *tt)
{
    trainSet = tr;
    testSet  = tt;

    int d = tr->dimension();

    problem_dimension = d;

    vclass = tr->getPatternClass();

    program =
        new Cprogram(
            d,
            vclass.size()-1
            );

    setStartSymbol(
        program->getStartSymbol()
        );

    nclass =
        vclass.size();

    pstring.resize(
        vclass.size()
        );

    for(unsigned int i=0;i<pstring.size();i++)
        pstring[i]=" ";

    pgenome.resize(nclass);

    outy.resize(
        trainSet->count()
        );

    trainx =
        trainSet->getAllXpoint();

    trainy =
        trainSet->getAllYPoints();

    // --------------------------------------------------------
    // Sort classes
    // --------------------------------------------------------

    for(int i=0;i<(int)vclass.size();i++)
    {
        for(int j=0;j<(int)vclass.size()-1;j++)
        {
            if(vclass[j+1]<vclass[j])
            {
                double d=vclass[j];

                vclass[j]=
                    vclass[j+1];

                vclass[j+1]=d;
            }
        }
    }

    mapper.resize(
        vclass.size()
        );

    for(int i=0;i<(int)vclass.size();i++)
    {
        mapper[i]=vclass[i];
        vclass[i]=i;
    }

    outy.resize(
        trainy.size()
        );
}

// ============================================================
// GET DIMENSION
//
// IMPORTANT:
// Keep const because population.cc expects:
//
// ClassProgram::getDimension() const
// ============================================================

int ClassProgram::getDimension() const
{
    return trainSet->dimension();
}

// ============================================================
// PRINT CLASSIFIER
// ============================================================

string ClassProgram::printF(
    vector<int> &genome
    )
{
    string ret="";

    if(nclass<=1)
        return "";

    if(
        pgenome.size()!=
        genome.size()/(nclass-1)
        )
    {
        pgenome.resize(
            genome.size()/(nclass-1)
            );
    }

    char str[100];

    extern int wrapping;

    for(int i=0;i<nclass-1;i++)
    {
        for(
            int j=0;
            j<(int)genome.size()/(nclass-1);
            j++
            )
        {
            pgenome[j]=
                genome[
                    i*
                        genome.size()/
                        (nclass-1)
                    +
                    j
            ];
        }

        int redo=0;

        pstring[i]=
            printRandomProgram(
                pgenome,
                redo
                );

        if(redo>=wrapping)
            return "";

        ret+="if(";
        ret+=pstring[i];
        ret+=") CLASS=";

        sprintf(
            str,
            "%.2lf",
            vclass[i]
            );

        ret+=str;
        ret+="\nelse \n";
    }

    sprintf(
        str,
        "%.2lf",
        vclass[nclass-1]
        );

    ret+="CLASS=";
    ret+=str;
    ret+="\n";

    return ret;
}

// ============================================================
// TARGETED GENOTYPE TRACE
//
// The complete classifier chromosome is divided into
// nclass-1 segments.
//
// Rule::printRule() gives positions relative to the local
// segment. Here we convert them to positions in the complete
// chromosome.
// ============================================================

void ClassProgram::getCodonTrace(
    vector<int> &genome,
    vector<CodonTrace> &trace
    )
{
    trace.clear();

    if(nclass<=1)
        return;

    if(genome.empty())
        return;

    const int numberOfRules =
        nclass-1;

    const int partSize =
        static_cast<int>(
            genome.size()
            ) /
        numberOfRules;

    if(partSize<=0)
        return;

    // --------------------------------------------------------
    // Get grammar start symbol
    // --------------------------------------------------------

    Symbol *start =
        getStartSymbol();

    if(start==NULL)
        return;

    if(start->getCountRules()<=0)
        return;

    Rule *startRule =
        start->getRule(0);

    if(startRule==NULL)
        return;

    // --------------------------------------------------------
    // Local genome
    // --------------------------------------------------------

    vector<int> localGenome;

    localGenome.resize(
        partSize
        );

    // ========================================================
    // Process every classifier rule
    // ========================================================

    for(
        int classRule=0;
        classRule<numberOfRules;
        classRule++
        )
    {
        const int offset =
            classRule *
            partSize;

        // ----------------------------------------------------
        // Copy this class segment
        // ----------------------------------------------------

        for(
            int j=0;
            j<partSize;
            j++
            )
        {
            localGenome[j]=
                genome[
                    offset+j
            ];
        }

        vector<CodonTrace>
            localTrace;

        int pos=0;
        int redo=0;

        // ----------------------------------------------------
        // Generate phenotype and trace simultaneously
        // ----------------------------------------------------

        startRule->printRule(
            localGenome,
            pos,
            redo,
            &localTrace
            );

        // ----------------------------------------------------
        // Convert local codon positions to global positions
        // ----------------------------------------------------

        for(
            size_t i=0;
            i<localTrace.size();
            i++
            )
        {
            CodonTrace item =
                localTrace[i];

            if(
                item.genomePos<0 ||
                item.genomePos>=partSize
                )
            {
                continue;
            }

            item.genomePos +=
                offset;

            if(
                item.genomePos<0 ||
                item.genomePos>=
                    (int)genome.size()
                )
            {
                continue;
            }

            trace.push_back(
                item
                );
        }
    }
}

// ============================================================
// PRINT PYTHON
// ============================================================

void ClassProgram::printPython(
    vector<int> &genome,
    std::string outname
    )
{
    std::regex e("\\.py$");

    std::string s =
        outname;

    s =
        std::regex_replace(
            s,
            e,
            ".c"
            );

    this->printC(
        genome,
        s
        );

    std::string strprogram(
        "import ctypes\n"
        "import numpy as np\n"
        "from typing import List\n\n"
        "def classifier(input: List):\n"
        "\tfun = ctypes.CDLL(\"./classifier.so\")\n"
        "\tfun.classifier.argtypes = [ctypes.POINTER(ctypes.c_double)]\n"
        "\tfun.classifier.restype = ctypes.c_int\n"
        "\ta = np.array(input)\n"
        "\tinput_ptr = a.ctypes.data_as(ctypes.POINTER(ctypes.c_double))\n"
        "\treturn fun.classifier(input_ptr)\n\n\n"
        );

    ofstream outprogram;

    outprogram.open(outname);

    if(!outprogram)
    {
        cerr
            << "Error: file could not be opened"
            << endl;

        exit(1);
    }

    outprogram <<
        strprogram;

    outprogram.close();
}

// ============================================================
// PRINT C
// ============================================================

void ClassProgram::printC(
    vector<int> &genome,
    std::string outname
    )
{
    ofstream outprogram;

    std::string strprogram(
        "#include <math.h>\n\n"
        "int classifier(double *x){\n\n"
        "\tint CLASS = 0;\n\n"
        );

    std::string s(
        printF(genome)
        );

    std::regex e(
        "x(\\d+)"
        );

    s =
        std::regex_replace(
            s,
            e,
            "x[$1]"
            );

    // --------------------------------------------------------
    // Decrement variable indices
    // --------------------------------------------------------

    for(
        int i=3;
        i<(int)s.size();
        i++
        )
    {
        int j=i;

        if(s[j]==']')
        {
            j--;

            while(
                j>=0 &&
                s[j]=='0'
                )
            {
                s[j]='9';
                j--;
            }

            if(j>=0)
                s[j]-=1;

            while(
                j>=0 &&
                j+1<(int)s.size() &&
                s[j]=='0' &&
                s[j+1]!=']'
                )
            {
                s.erase(j,1);
            }
        }
    }

    e =
        std::regex(
            "\nelse \nif"
            );

    s =
        std::regex_replace(
            s,
            e,
            "\nelse if"
            );

    e =
        std::regex(
            "\nelse \n"
            );

    s =
        std::regex_replace(
            s,
            e,
            "\nelse "
            );

    e =
        std::regex(
            "(\\d+)\\.(\\d+)\n"
            );

    s =
        std::regex_replace(
            s,
            e,
            "$1;\n"
            );

    e =
        std::regex("\\&");

    s =
        std::regex_replace(
            s,
            e,
            "&&"
            );

    e =
        std::regex("\\|");

    s =
        std::regex_replace(
            s,
            e,
            "||"
            );

    e =
        std::regex("\n");

    s =
        std::regex_replace(
            s,
            e,
            "\n\t"
            );

    s =
        string("\t")+
        s;

    strprogram =
        strprogram+
        s+
        "\n\treturn CLASS;\n}\n";

    outprogram.open(
        outname
        );

    if(!outprogram)
    {
        cerr
            << "Error: file could not be opened"
            << endl;

        exit(1);
    }

    outprogram <<
        strprogram;

    outprogram.close();
}

// ============================================================
// PRECISION / RECALL - TRAIN SET
// ============================================================

void ClassProgram::getPrecisionAndRecall(
    double &precision,
    double &recall,
    double &macroF1,
    double &weightedF1,
    double &gmean
    )
{
    getPrecisionAndRecall(
        trainSet,
        precision,
        recall,
        macroF1,
        weightedF1,
        gmean
        );
}

// ============================================================
// PRECISION / RECALL
// ============================================================

void ClassProgram::getPrecisionAndRecall(
    Dataset *t,
    double &precision,
    double &recall,
    double &macroF1,
    double &weightedF1,
    double &gmean
    )
{
    int i,j;

    getOutputs(
        t,
        realCached,
        estCached
        );

    int N =
        realCached.size();

    // IMPORTANT:
    // Class count always comes from TRAIN SET.
    int classCount =
        getClass();

    int **CM;

    CM =
        new int*[
            classCount
    ];

    for(i=0;i<classCount;i++)
    {
        CM[i]=
            new int[
                classCount
        ];
    }

    for(i=0;i<classCount;i++)
    {
        for(j=0;j<classCount;j++)
        {
            CM[i][j]=0;
        }
    }

    // --------------------------------------------------------
    // Confusion matrix
    // --------------------------------------------------------

    for(i=0;i<N;i++)
    {
        int r =
            (int)realCached[i];

        int e =
            (int)estCached[i];

        if(
            r>=0 &&
            r<classCount &&
            e>=0 &&
            e<classCount
            )
        {
            CM[r][e]++;
        }
    }

    Data recallArray;
    Data precisionArray;

    precisionArray.resize(
        classCount
        );

    recallArray.resize(
        classCount
        );

    for(i=0;i<classCount;i++)
    {
        double sum=0.0;

        for(j=0;j<classCount;j++)
            sum+=CM[j][i];

        precisionArray[i]=
            sum==0
                ?
                -1.0
                :
                CM[i][i]/sum;

        sum=0.0;

        for(j=0;j<classCount;j++)
            sum+=CM[i][j];

        recallArray[i]=
            sum==0
                ?
                -1.0
                :
                CM[i][i]/sum;
    }

    double avg_precision=0.0;
    double avg_recall=0.0;

    int total_classes1=
        classCount;

    int total_classes2=
        classCount;

    for(i=0;i<classCount;i++)
    {
        if(precisionArray[i]<0)
            total_classes1--;
        else
            avg_precision+=
                precisionArray[i];

        if(recallArray[i]<0)
            total_classes2--;
        else
            avg_recall+=
                recallArray[i];
    }

    if(total_classes1>0)
        avg_precision/=
            total_classes1;

    if(total_classes2>0)
        avg_recall/=
            total_classes2;

    precision =
        avg_precision;

    recall =
        avg_recall;

    for(i=0;i<classCount;i++)
        delete[] CM[i];

    delete[] CM;

    macroF1=0.0;
    weightedF1=0.0;
    gmean=0.0;

    vector<int> belong;

    belong.resize(
        classCount,
        0
        );

    /*
     * IMPORTANT:
     *
     * Use the supplied Dataset t here.
     *
     * This means that if a class exists in TRAIN but not TEST,
     * belong[i] will simply remain zero.
     */

    Data metricY =
        t->getAllYPoints();

    for(
        unsigned int i=0;
        i<metricY.size();
        i++
        )
    {
        int pos =
            findMapper(
                metricY[i]
                );

        if(
            pos>=0 &&
            pos<classCount
            )
        {
            belong[pos]++;
        }
    }

    int total_class=0;

    double gmeanLog=0.0;

    for(i=0;i<classCount;i++)
    {
        /*
         * No examples of this class in this particular set:
         * skip it from macro metrics.
         */
        if(
            belong[i]==0 ||
            recallArray[i]<0
            )
        {
            continue;
        }

        double p =
            precisionArray[i];

        double r =
            recallArray[i];

        if(p<0)
            p=0.0;

        double denominator =
            p+r;

        double f1 =
            denominator>0.0
                ?
                2.0*p*r/denominator
                :
                0.0;

        macroF1 +=
            f1;

        /*
         * Small epsilon prevents log(0).
         */
        gmeanLog +=
            log(
                r+
                0.001
                );

        if(!metricY.empty())
        {
            weightedF1 +=
                belong[i] *
                1.0 /
                metricY.size() *
                f1;
        }

        total_class++;
    }

    if(total_class>0)
    {
        macroF1 /=
            total_class;

        gmean =
            exp(
                gmeanLog /
                total_class
                );
    }
    else
    {
        macroF1=0.0;
        gmean=0.0;
    }
}

// ============================================================
// FIND CLASS MAPPER
// ============================================================

int ClassProgram::findMapper(
    double y
    )
{
    for(
        unsigned int i=0;
        i<mapper.size();
        i++
        )
    {
        if(
            fabs(
                mapper[i]-y
                )<1e-7
            )
        {
            return i;
        }
    }

    return 0;
}

// ============================================================
// TEST CLASS ERROR
// ============================================================

double ClassProgram::getClassError(
    vector<int> &genome
    )
{
    Matrix testx =
        testSet->getAllXpoint();

    Data testy =
        testSet->getAllYPoints();

    double value=0.0;

    if(nclass<=1)
        return 1e+8;

    if(
        pgenome.size()!=
        genome.size()/(nclass-1)
        )
    {
        pgenome.resize(
            genome.size()/(nclass-1)
            );
    }

    if(
        outy.size()!=
        testy.size()
        )
    {
        outy.resize(
            testy.size()
            );
    }

    for(
        unsigned int i=0;
        i<outy.size();
        i++
        )
    {
        outy[i]=
            NAN_CLASS;
    }

    for(
        int i=0;
        i<nclass-1;
        i++
        )
    {
        for(
            unsigned int j=0;
            j<pgenome.size();
            j++
            )
        {
            pgenome[j]=
                genome[
                    i*
                        genome.size()/
                        (nclass-1)
                    +
                    j
            ];
        }

        int redo=0;

        string s =
            printRandomProgram(
                pgenome,
                redo
                );

        if(redo>=wrapping)
            return 1e+8;

        pstring[i]=s;
    }

    for(
        int j=0;
        j<nclass-1;
        j++
        )
    {
        program->Parse(
            pstring[j]
            );

        for(
            unsigned int i=0;
            i<testy.size();
            i++
            )
        {
            if(
                fabs(
                    outy[i]-
                    NAN_CLASS
                    )>1e-5
                )
            {
                continue;
            }

            double v =
                program->Eval(
                    testx[i].data()
                    );

            if(
                isnan(v) ||
                isinf(v)
                )
            {
                return 1e+8;
            }

            if(
                fabs(
                    v-1.0
                    )<1e-5
                )
            {
                outy[i]=
                    vclass[j];
            }
        }
    }

    vector<int> fail;
    vector<int> belong;

    fail.resize(
        nclass,
        0
        );

    belong.resize(
        nclass,
        0
        );

    for(
        unsigned int i=0;
        i<testy.size();
        i++
        )
    {
        if(
            fabs(
                outy[i]-
                NAN_CLASS
                )<1e-5
            )
        {
            outy[i]=
                vclass[nclass-1];
        }

        int actual =
            findMapper(
                testy[i]
                );

        value +=
            (
                fabs(
                    actual-
                    outy[i]
                    )>1e-5
                );

        if(
            actual>=0 &&
            actual<nclass
            )
        {
            belong[actual]++;

            if(
                fabs(
                    actual-
                    outy[i]
                    )>1e-5
                )
            {
                fail[actual]++;
            }
        }
    }

    printf(
        "TEST REPORT=>\n"
        );

    for(
        int i=0;
        i<nclass;
        i++
        )
    {
        /*
         * A train class may be absent from the test set.
         * That is valid and must not terminate evaluation.
         */
        if(belong[i]==0)
            continue;

        printf(
            "CLASS[%3d (%3d)] FAIL=%5.2lf%% \n",
            i,
            belong[i],
            fail[i]*100.0/belong[i]
            );
    }

    if(
        isnan(value) ||
        isinf(value)
        )
    {
        return 1e+8;
    }

    if(testy.empty())
        return 0.0;

    return
        -value*
        100.0/
        testy.size();
}

// ============================================================
// GET CLASS COUNT
//
// IMPORTANT:
// Keep const because population.cc expects:
//
// ClassProgram::getClass() const
// ============================================================

int ClassProgram::getClass() const
{
    return nclass;
}

// ============================================================
// FITNESS
// ============================================================

double ClassProgram::fitness(
    vector<int> &genome
    )
{
    if(nclass<=1)
        return 1e+8;

    outy.resize(
        trainy.size()
        );

    pgenome.resize(
        genome.size()/
        (nclass-1)
        );

    double value=0.0;

    for(
        unsigned int i=0;
        i<outy.size();
        i++
        )
    {
        outy[i]=
            NAN_CLASS;
    }

    extern int wrapping;

    for(
        int i=0;
        i<nclass-1;
        i++
        )
    {
        for(
            unsigned int j=0;
            j<pgenome.size();
            j++
            )
        {
            pgenome[j]=
                genome[
                    i*
                        genome.size()/
                        (nclass-1)
                    +
                    j
            ];
        }

        int redo=0;

        string s =
            printRandomProgram(
                pgenome,
                redo
                );

        if(redo>=wrapping)
            return 1e+8;

        pstring[i]=s;
    }

    for(
        int j=0;
        j<nclass-1;
        j++
        )
    {
        int d =
            program->Parse(
                pstring[j]
                );

        if(!d)
            return 1e+8;

        for(
            unsigned int i=0;
            i<trainy.size();
            i++
            )
        {
            if(
                fabs(
                    outy[i]-
                    NAN_CLASS
                    )>1e-5
                )
            {
                continue;
            }

            double v =
                program->Eval(
                    trainx[i].data()
                    );

            if(
                program->EvalError()
                )
            {
                return 1e+8;
            }

            if(
                isnan(v) ||
                isinf(v)
                )
            {
                return 1e+8;
            }

            if(
                fabs(
                    v-1.0
                    )<1e-5
                )
            {
                outy[i]=
                    vclass[j];
            }
        }
    }

    vector<int> fail;
    vector<int> belong;

    fail.resize(
        nclass,
        0
        );

    belong.resize(
        nclass,
        0
        );

    for(
        unsigned int i=0;
        i<trainy.size();
        i++
        )
    {
        if(
            fabs(
                outy[i]-
                NAN_CLASS
                )<1e-5
            )
        {
            outy[i]=
                vclass[nclass-1];
        }

        int pos =
            findMapper(
                trainy[i]
                );

        value +=
            (
                fabs(
                    pos-
                    outy[i]
                    )>1e-5
                );

        if(
            pos>=0 &&
            pos<nclass
            )
        {
            belong[pos]++;

            if(
                fabs(
                    pos-
                    outy[i]
                    )>1e-5
                )
            {
                fail[pos]++;
            }
        }
    }

    double value1=0.0;
    double value2=0.0;

    double value1_max=0.0;

    int validClasses=0;

    for(
        int i=0;
        i<nclass;
        i++
        )
    {
        /*
         * Defensive check.
         * All train classes normally have examples, but avoid
         * division by zero if a malformed dataset is supplied.
         */
        if(belong[i]==0)
            continue;

        double f =
            fail[i]*
            100.0/
            belong[i];

        value1 +=
            f;

        value2 +=
            f*f;

        if(f>value1_max)
            value1_max=f;

        validClasses++;
    }

    if(
        isnan(value) ||
        isinf(value)
        )
    {
        return 1e+8;
    }

    if(fitness_mode==FITNESS_CLASS)
    {
        if(trainy.empty())
            return 1e+8;

        return
            value*
            100.0/
            trainy.size();
    }

    else if(
        fitness_mode==
        FITNESS_AVERAGE
        )
    {
        if(validClasses==0)
            return 1e+8;

        return
            value1/
            validClasses;
    }

    else if(
        fitness_mode==
        FITNESS_SQUARED
        )
    {
        if(validClasses==0)
            return 1e+8;

        return
            sqrt(
                value2/
                validClasses
                );
    }

    else if(
        fitness_mode==
        FITNESS_MIXED
        )
    {
        if(
            trainy.empty() ||
            validClasses==0
            )
        {
            return 1e+8;
        }

        return
            class_percent*
                (
                    value*
                    100.0/
                    trainy.size()
                    )
            +
            average_percent*
                (
                    value1/
                    validClasses
                    )
            +
            squared_percent*
                sqrt(
                    value2/
                    validClasses
                    );
    }

    else if(
        fitness_mode==
        FITNESS_MEAN
        )
    {
        double precision=0.0;
        double recall=0.0;
        double macroF1=0.0;
        double weightedF1=0.0;
        double gmean=0.0;

        getPrecisionAndRecall(
            precision,
            recall,
            macroF1,
            weightedF1,
            gmean
            );

        return
            100.0-
            100.0*gmean;
    }

    else if(
        fitness_mode==
        FITNESS_MACRO_F1
        )
    {
        double precision=0.0;
        double recall=0.0;
        double macroF1=0.0;
        double weightedF1=0.0;
        double gmean=0.0;

        getPrecisionAndRecall(
            precision,
            recall,
            macroF1,
            weightedF1,
            gmean
            );

        return
            100.0-
            100.0*macroF1;
    }

    else if(
        fitness_mode==
        FITNESS_WEIGHTED_F1
        )
    {
        double precision=0.0;
        double recall=0.0;
        double macroF1=0.0;
        double weightedF1=0.0;
        double gmean=0.0;

        getPrecisionAndRecall(
            precision,
            recall,
            macroF1,
            weightedF1,
            gmean
            );

        return
            100.0-
            100.0*weightedF1;
    }

    return 0.0;
}

// ============================================================
// ERROR PER CLASS
// ============================================================

void ClassProgram::getErrorPerClass(
    vector<int> &genome,
    vector<double> &x
    )
{
    if(nclass<=1)
        return;

    outy.resize(
        trainy.size()
        );

    pgenome.resize(
        genome.size()/
        (nclass-1)
        );

    x.resize(
        nclass
        );

    for(int i=0;i<nclass;i++)
        x[i]=100.0;

    for(
        unsigned int i=0;
        i<outy.size();
        i++
        )
    {
        outy[i]=
            NAN_CLASS;
    }

    extern int wrapping;

    for(
        int i=0;
        i<nclass-1;
        i++
        )
    {
        for(
            unsigned int j=0;
            j<pgenome.size();
            j++
            )
        {
            pgenome[j]=
                genome[
                    i*
                        genome.size()/
                        (nclass-1)
                    +
                    j
            ];
        }

        int redo=0;

        string s =
            printRandomProgram(
                pgenome,
                redo
                );

        if(redo>=wrapping)
            return;

        pstring[i]=s;
    }

    for(
        int j=0;
        j<nclass-1;
        j++
        )
    {
        int d =
            program->Parse(
                pstring[j]
                );

        if(!d)
            return;

        for(
            unsigned int i=0;
            i<trainy.size();
            i++
            )
        {
            if(
                fabs(
                    outy[i]-
                    NAN_CLASS
                    )>1e-5
                )
            {
                continue;
            }

            double v =
                program->Eval(
                    trainx[i].data()
                    );

            if(
                program->EvalError()
                )
            {
                return;
            }

            if(
                isnan(v) ||
                isinf(v)
                )
            {
                return;
            }

            if(
                fabs(
                    v-1.0
                    )<1e-5
                )
            {
                outy[i]=
                    vclass[j];
            }
        }
    }

    vector<int> fail;
    vector<int> belong;

    fail.resize(
        nclass,
        0
        );

    belong.resize(
        nclass,
        0
        );

    for(
        unsigned int i=0;
        i<trainy.size();
        i++
        )
    {
        if(
            fabs(
                outy[i]-
                NAN_CLASS
                )<1e-5
            )
        {
            outy[i]=
                vclass[nclass-1];
        }

        int pos =
            findMapper(
                trainy[i]
                );

        if(
            pos>=0 &&
            pos<nclass
            )
        {
            belong[pos]++;

            if(
                fabs(
                    pos-
                    outy[i]
                    )>1e-5
                )
            {
                fail[pos]++;
            }
        }
    }

    for(
        int i=0;
        i<nclass;
        i++
        )
    {
        if(belong[i]==0)
        {
            x[i]=0.0;
            continue;
        }

        x[i]=
            fail[i]*
            100.0/
            belong[i];
    }
}

// ============================================================
// FITNESS MODE
// ============================================================

void ClassProgram::setFitnessMode(
    int m
    )
{
    fitness_mode=m;
}

// ============================================================
// FITNESS PERCENTAGES
// ============================================================

void ClassProgram::setFitnessPercentages(
    double p1,
    double p2,
    double p3
    )
{
    class_percent =
        p1>=0 &&
                p1<=1.0
            ?
            p1
            :
            class_percent;

    average_percent =
        p2>=0 &&
                p2<=1.0
            ?
            p2
            :
            average_percent;

    squared_percent =
        p3>=0 &&
                p3<=1.0
            ?
            p3
            :
            squared_percent;
}

// ============================================================
// GET OUTPUTS
// ============================================================

void ClassProgram::getOutputs(
    Dataset *t,
    vector<double> &real,
    vector<double> &est
    )
{
    Data testy =
        t->getAllYPoints();

    if(
        real.size()!=
        testy.size()
        )
    {
        real.resize(
            testy.size()
            );

        est.resize(
            testy.size()
            );
    }

    /*
     * IMPORTANT:
     *
     * Class mapping comes from the TRAIN classes through
     * mapper/findMapper().
     *
     * Therefore TEST is allowed to contain fewer classes.
     */

    for(
        unsigned int i=0;
        i<testy.size();
        i++
        )
    {
        real[i]=
            findMapper(
                testy[i]
                );

        if(i<outy.size())
            est[i]=outy[i];
        else
            est[i]=0.0;
    }
}

// ============================================================
// GET TRAIN OUTPUTS
// ============================================================

void ClassProgram::getOutputs(
    vector<double> &real,
    vector<double> &est
    )
{
    getOutputs(
        trainSet,
        real,
        est
        );
}

// ============================================================
// DESTRUCTOR
// ============================================================

ClassProgram::~ClassProgram()
{
    delete program;
}
