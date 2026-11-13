#include <GE/classprogram.h>

#include <math.h>
#include <cmath>

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
#include <algorithm>
#include <random>
#include <numeric>

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
    program =new Cprogram(d,vclass.size()-1);
    setStartSymbol(program->getStartSymbol());

    nclass =vclass.size();

    pstring.resize(vclass.size());

    for(unsigned int i=0;i<pstring.size();i++)
        pstring[i]=" ";

    pgenome.resize(nclass);
    outy.resize(trainSet->count());
    trainx =trainSet->getAllXpoint();

    trainy =trainSet->getAllYPoints();

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
                vclass[j]=vclass[j+1];
                vclass[j+1]=d;
            }
        }
    }

    mapper.resize(vclass.size());

    for(int i=0;i<(int)vclass.size();i++)
    {
        mapper[i]=vclass[i];
        vclass[i]=i;
    }

    outy.resize(trainy.size());
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

string ClassProgram::printF(vector<int> &genome)
{
    string ret="";
    if(nclass<=1) return "";

    if(pgenome.size()!=genome.size()/(nclass-1))
    {
        pgenome.resize(genome.size()/(nclass-1));
    }

    char str[100];
    extern int wrapping;
    for(int i=0;i<nclass-1;i++)
    {
        for(int j=0;j<(int)genome.size()/(nclass-1);j++)
        {
            pgenome[j]=genome[i*genome.size()/(nclass-1)+j];
        }
        int redo=0;
        pstring[i]=
            printRandomProgram(pgenome,redo);

        if(redo>=wrapping)return "";

        ret+="if(";
        ret+=pstring[i];
        ret+=") CLASS=";

        sprintf(str,"%.2lf",vclass[i]);

        ret+=str;
        ret+="\nelse \n";
    }

    sprintf(str,"%.2lf",vclass[nclass-1]);
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

void ClassProgram::getCodonTrace(vector<int> &genome,
            vector<CodonTrace> &trace)
{
    trace.clear();
    if(nclass<=1)
        return;
    if(genome.empty())
        return;
    const int numberOfRules =nclass-1;

    const int partSize =static_cast<int>(genome.size())/numberOfRules;

    if(partSize<=0)return;

    // --------------------------------------------------------
    // Get grammar start symbol
    // --------------------------------------------------------

    Symbol *start =
        getStartSymbol();

    if(start==NULL)
        return;

    if(start->getCountRules()<=0)
        return;

    Rule *startRule =start->getRule(0);

    if(startRule==NULL) return;

    // --------------------------------------------------------
    // Local genome
    // --------------------------------------------------------

    vector<int> localGenome;

    localGenome.resize(partSize);

    // ========================================================
    // Process every classifier rule
    // ========================================================

    for(int classRule=0;classRule<numberOfRules;classRule++)
    {
        const int offset =classRule *partSize;

        // ----------------------------------------------------
        // Copy this class segment
        // ----------------------------------------------------
        for(int j=0;j<partSize;j++)
        {
            localGenome[j]=genome[offset+j];
        }
        vector<CodonTrace>localTrace;

        int pos=0;
        int redo=0;

        // ----------------------------------------------------
        // Generate phenotype and trace simultaneously
        // ----------------------------------------------------

        startRule->printRule(localGenome,pos,redo,&localTrace);

        // ----------------------------------------------------
        // Convert local codon positions to global positions
        // ----------------------------------------------------

        for(size_t i=0;i<localTrace.size();i++)
        {
            CodonTrace item=localTrace[i];
            if(item.genomePos<0 ||item.genomePos>=partSize)
            {
                continue;
            }
            item.genomePos +=offset;
            if(item.genomePos<0 ||item.genomePos>=(int)genome.size())
            {
                continue;
            }
            trace.push_back(item);
        }
    }
}

// ============================================================
// PRINT PYTHON
// ============================================================

void ClassProgram::printPython(vector<int> &genome,std::string outname)
{
    std::regex e("\\.py$");

    std::string s =outname;

    s =std::regex_replace(s,e,".c");

    this->printC(genome,s);

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
    outprogram <<strprogram;
    outprogram.close();
}

// ============================================================
// PRINT C
// ============================================================

void ClassProgram::printC(vector<int> &genome,std::string outname)
{
    ofstream outprogram;

    std::string strprogram(
        "#include <math.h>\n"
        "#include <cmath>\n\n"
        "int classifier(double *x){\n\n"
        "\tint CLASS = 0;\n\n"
        );

    std::string s(
        printF(genome)
        );

    std::regex e(
        "x(\\d+)"
        );

    s =std::regex_replace(s,e,"x[$1]");

    // --------------------------------------------------------
    // Decrement variable indices
    // --------------------------------------------------------

    for(int i=3;i<(int)s.size();i++)
    {
        int j=i;

        if(s[j]==']')
        {
            j--;
            while(j>=0 &&s[j]=='0')
            {
                s[j]='9';
                j--;
            }

            if(j>=0)
                s[j]-=1;

            while(j>=0 &&j+1<(int)s.size() &&s[j]=='0' &&s[j+1]!=']')
            {
                s.erase(j,1);
            }
        }
    }

    e =std::regex("\nelse \nif");
    s =std::regex_replace(s,e,"\nelse if");
    e =std::regex("\nelse \n");

    s =std::regex_replace(s,e,"\nelse ");
    e =std::regex("(\\d+)\\.(\\d+)\n");
    s =std::regex_replace(s,e,"$1;\n");
    e =std::regex("\\&");

    s =std::regex_replace(s,e,"&&");
    e =std::regex("\\|");
    s =std::regex_replace(s,e,"||");
    e =std::regex("\n");
    s =std::regex_replace(s,e,"\n\t");
    s =string("\t")+s;

    strprogram =strprogram+s+"\n\treturn CLASS;\n}\n";

    outprogram.open(outname);

    if(!outprogram)
    {
        cerr
            << "Error: file could not be opened"
            << endl;

        exit(1);
    }

    outprogram <<strprogram;
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

    getOutputs(t,realCached,estCached);

    int N =realCached.size();

    // IMPORTANT:
    // Class count always comes from TRAIN SET.
    int classCount =getClass();

    int **CM;
    CM =new int*[classCount];

    for(i=0;i<classCount;i++)
    {
        CM[i]=new int[classCount];
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
        int r =(int)realCached[i];
        int e =(int)estCached[i];

        if(r>=0 &&r<classCount &&e>=0 &&e<classCount)
        {
            CM[r][e]++;
        }
    }

    // Fixed training-class universe for all macro metrics.
    // Zero-denominator precision and recall are defined as zero.
    Data recallArray(classCount,0.0);
    Data precisionArray(classCount,0.0);
    vector<int> support(classCount,0);
    for(i=0;i<classCount;i++)
    {
        double predicted=0.0, actual=0.0;
        for(j=0;j<classCount;j++)
        {
            predicted+=CM[j][i];
            actual+=CM[i][j];
        }
        precisionArray[i]=predicted>0.0?CM[i][i]/predicted:0.0;
        recallArray[i]=actual>0.0?CM[i][i]/actual:0.0;
        support[i]=(int)actual;
    }

    precision=0.0;
    recall=0.0;
    macroF1=0.0;
    weightedF1=0.0;
    gmean=0.0;
    double supportTotal=0.0;
    bool allRecallsPositive=(classCount>0);
    double gmeanLog=0.0;
    for(i=0;i<classCount;i++)
    {
        const double p=precisionArray[i];
        const double r=recallArray[i];
        const double f1=(p+r>0.0)?2.0*p*r/(p+r):0.0;
        precision+=p;
        recall+=r;
        macroF1+=f1;
        weightedF1+=support[i]*f1;
        supportTotal+=support[i];
        if(r<=0.0) allRecallsPositive=false;
        else gmeanLog+=log(r);
    }
    if(classCount>0)
    {
        precision/=classCount;
        recall/=classCount;
        macroF1/=classCount;
        if(allRecallsPositive) gmean=exp(gmeanLog/classCount);
    }
    if(supportTotal>0.0) weightedF1/=supportTotal;

    for(i=0;i<classCount;i++) delete[] CM[i];
    delete[] CM;
}

// ============================================================
// FIND CLASS MAPPER
// ============================================================

int ClassProgram::findMapper(double y)
{
    for(unsigned int i=0;i<mapper.size();i++)
    {
        if(fabs(mapper[i]-y)<1e-7)
        {
            return i;
        }
    }
    return 0;
}

// ============================================================
// TEST CLASS ERROR
// ============================================================

double ClassProgram::getClassError(vector<int> &genome)
{
    Matrix testx =testSet->getAllXpoint();
    Data testy =testSet->getAllYPoints();

    double value=0.0;
    if(nclass<=1)return 1e+8;

    if(pgenome.size()!=genome.size()/(nclass-1))
    {
        pgenome.resize(genome.size()/(nclass-1));
    }

    if(outy.size()!=testy.size())
    {
        outy.resize(testy.size());
    }

    for(unsigned int i=0;i<outy.size();i++)
    {
        outy[i]=NAN_CLASS;
    }

    for(int i=0;i<nclass-1;i++)
    {
        for(unsigned int j=0;j<pgenome.size();j++)
        {
            pgenome[j]=genome[i*genome.size()/(nclass-1)+j];
        }

        int redo=0;
        string s =printRandomProgram(pgenome,redo);

        if(redo>=wrapping)return 1e+8;
        pstring[i]=s;
    }

    for(int j=0;j<nclass-1;j++)
    {
        program->Parse(pstring[j]);

        for(unsigned int i=0;i<testy.size();i++)
        {
            if(fabs(outy[i]-NAN_CLASS)>1e-5)
            {
                continue;
            }

            double v =program->Eval(testx[i].data());

            if(isnan(v) ||isinf(v))
            {
                return 1e+8;
            }

            if(fabs(v-1.0)<1e-5)
            {
                outy[i]=vclass[j];
            }
        }
    }

    vector<int> fail;
    vector<int> belong;

    fail.resize(nclass,0);
    belong.resize(nclass,0);

    for(unsigned int i=0;i<testy.size();i++)
    {
        if(fabs(outy[i]-NAN_CLASS)<1e-5)
        {
            outy[i]=vclass[nclass-1];
        }
        int actual =findMapper(testy[i]);
        value +=(fabs(actual-outy[i])>1e-5);

        if(actual>=0 &&actual<nclass)
        {
            belong[actual]++;
            if(fabs(actual-outy[i])>1e-5)
            {
                fail[actual]++;
            }
        }
    }

    if(!quietTestReport) printf("TEST REPORT=>\n");

    for(int i=0;i<nclass;i++)
    {
        /*
         * A train class may be absent from the test set.
         * That is valid and must not terminate evaluation.
         */
        if(belong[i]==0)continue;

        if(!quietTestReport) printf(
            "CLASS[%3d (%3d)] FAIL=%5.2lf%% \n",
            i, belong[i], fail[i]*100.0/belong[i]
            );
    }

    if(isnan(value) ||isinf(value))
    {
        return 1e+8;
    }

    if(testy.empty()) return 0.0;

    return -value*100.0/testy.size();
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

void ClassProgram::configureBalancedSampling(int interval,int ratio,int minorityN,unsigned seed)
{
    sampleInterval=std::max(1,interval);
    sampleMajorityRatio=std::max(1,ratio);
    sampleMinorityN=std::max(0,minorityN);
    sampleSeed=seed;
    lastSampleBlock=-1;
    balancedSample.clear();
}

void ClassProgram::setSamplingLambda(double lambda)
{
    samplingLambda=std::max(0.0,std::min(1.0,lambda));
}

void ClassProgram::updateBalancedSampling(int generation)
{
    if(fitness_mode!=FITNESS_BALANCED_SAMPLING && fitness_mode!=FITNESS_HYBRID_SAMPLING)return;
    const int block=generation/sampleInterval;
    if(block==lastSampleBlock)return;
    lastSampleBlock=block;
    balancedSample.clear();
    std::vector<std::vector<int>> byClass(nclass);
    for(size_t i=0;i<trainy.size();++i) {
        const int c=findMapper(trainy[i]);
        if(c>=0 && c<nclass)byClass[c].push_back(static_cast<int>(i));
    }
    if(nclass!=2 || byClass[0].empty() || byClass[1].empty())return;
    const int minority=(byClass[0].size()<=byClass[1].size()?0:1);
    const int majority=1-minority;
    std::mt19937 rng(sampleSeed+static_cast<unsigned>(block)*2654435761u);
    std::shuffle(byClass[minority].begin(),byClass[minority].end(),rng);
    std::shuffle(byClass[majority].begin(),byClass[majority].end(),rng);
    const int nm=(sampleMinorityN==0?static_cast<int>(byClass[minority].size()):
        std::min(sampleMinorityN,static_cast<int>(byClass[minority].size())));
    const int nmaj=std::min(static_cast<int>(byClass[majority].size()),nm*sampleMajorityRatio);
    balancedSample.insert(balancedSample.end(),byClass[minority].begin(),byClass[minority].begin()+nm);
    balancedSample.insert(balancedSample.end(),byClass[majority].begin(),byClass[majority].begin()+nmaj);
}

// ============================================================
// FITNESS
// ============================================================

double ClassProgram::fitness(vector<int> &genome)
{
    if(nclass<=1) return 1e+8;

    outy.resize(trainy.size());

    pgenome.resize(genome.size()/(nclass-1));

    double value=0.0;

    for(unsigned int i=0;i<outy.size();i++)
    {
        outy[i]=NAN_CLASS;
    }

    extern int wrapping;
    for(int i=0;i<nclass-1;i++)
    {
        for(unsigned int j=0;j<pgenome.size();j++)
        {
            pgenome[j]=genome[i*genome.size()/(nclass-1)+j];
        }

        int redo=0;
        string s =printRandomProgram(pgenome,redo);

        if(redo>=wrapping)return 1e+8;
        pstring[i]=s;
    }

    for(int j=0;j<nclass-1;j++)
    {
        int d =program->Parse(pstring[j]);

        if(!d)return 1e+8;

        for(unsigned int i=0;i<trainy.size();i++)
        {
            if(fabs(outy[i]-NAN_CLASS)>1e-5)
            {
                continue;
            }

            double v =program->Eval(trainx[i].data());

            if(program->EvalError())
            {
                return 1e+8;
            }

            if(isnan(v) ||isinf(v))
            {
                return 1e+8;
            }
            if(fabs(v-1.0)<1e-5)
            {
                outy[i]=vclass[j];
            }
        }
    }

    vector<int> fail;
    vector<int> belong;
    fail.resize(nclass,0);
    belong.resize(nclass,0);

    for(unsigned int i=0;i<trainy.size();i++)
    {
        if(fabs(outy[i]-NAN_CLASS)<1e-5)
        {
            outy[i]=vclass[nclass-1];
        }

        int pos =findMapper(trainy[i]);
        value +=(fabs(pos-outy[i])>1e-5);

        if(pos>=0 &&pos<nclass)
        {
            belong[pos]++;
            if(fabs(pos-outy[i])>1e-5)
            {
                fail[pos]++;
            }
        }
    }

    double value1=0.0;
    double value2=0.0;
    double value1_max=0.0;
    int validClasses=0;

    for(int i=0;i<nclass;i++)
    {
        /*
         * Defensive check.
         * All train classes normally have examples, but avoid
         * division by zero if a malformed dataset is supplied.
         */
        if(belong[i]==0)continue;

        double f =fail[i]*100.0/belong[i];

        value1 +=f;
        value2 +=f*f;
        if(f>value1_max)
            value1_max=f;
        validClasses++;
    }

    if(isnan(value) ||isinf(value))
    {
        return 1e+8;
    }

    if(fitness_mode==FITNESS_BALANCED_SAMPLING || fitness_mode==FITNESS_HYBRID_SAMPLING)
    {
        // Balanced class error on a common subset, NOT raw subset accuracy.
        // Predictions are computed on all training rows above; only scoring is sampled.
        if(balancedSample.empty())return 1e+8;
        double errors[2]={0.0,0.0}, counts[2]={0.0,0.0};
        for(int i:balancedSample) {
            const int c=findMapper(trainy[i]);
            if(c<0 || c>=2)continue;
            counts[c]+=1.0;
            if(fabs(c-outy[i])>1e-5)errors[c]+=1.0;
        }
        if(counts[0]==0.0 || counts[1]==0.0)return 1e+8;
        const double sampledError=50.0*(errors[0]/counts[0]+errors[1]/counts[1]);
        if(fitness_mode==FITNESS_BALANCED_SAMPLING)return sampledError;
        const double fullError=100.0*value/static_cast<double>(trainy.size());
        return samplingLambda*sampledError+(1.0-samplingLambda)*fullError;
    }

    // Direct minority-class F1 objective on the TRAINING partition only.
    // This is a loss: smaller is better, with 0 being perfect F1.
    if(fitness_mode==FITNESS_MINORITY_F1)
    {
        int minority=-1;
        for(int c=0;c<nclass;c++) {
            if(belong[c]==0) continue;
            if(minority<0 || belong[c]<belong[minority]) minority=c;
        }
        if(minority<0) return 1e+8;
        double tp=0.0, fp=0.0, fn=0.0;
        for(size_t i=0;i<trainy.size();++i) {
            const int actual=findMapper(trainy[i]);
            const bool positive=(actual==minority);
            const bool predicted=(fabs(outy[i]-minority)<1e-5);
            if(positive && predicted) tp+=1.0;
            else if(!positive && predicted) fp+=1.0;
            else if(positive && !predicted) fn+=1.0;
        }
        const double denom=2.0*tp+fp+fn;
        const double f1=denom>0.0 ? 2.0*tp/denom : 0.0;
        return 100.0*(1.0-f1);
    }

    if(fitness_mode==FITNESS_CLASS)
    {
        if(trainy.empty())
            return 1e+8;
        return value*100.0/trainy.size();
    }
    else if(fitness_mode==FITNESS_AVERAGE)
    {
        if(validClasses==0) return 1e+8;
        return value1/validClasses;
    }

    else if(fitness_mode==FITNESS_SQUARED)
    {
        if(validClasses==0) return 1e+8;
        return sqrt(value2/validClasses);
    }

    else if(fitness_mode==FITNESS_MIXED)
    {
        if(trainy.empty() ||validClasses==0)
        {
            return 1e+8;
        }

        return
            class_percent*(value*100.0/trainy.size())
            +average_percent*(value1/validClasses)
            +squared_percent*sqrt(value2/validClasses);
    }
    else if(fitness_mode==FITNESS_MEAN)
    {
        double precision=0.0;
        double recall=0.0;
        double macroF1=0.0;
        double weightedF1=0.0;
        double gmean=0.0;

        getPrecisionAndRecall(precision,recall,macroF1,weightedF1,gmean);
        return 100.0-100.0*gmean;
    }
    else if(fitness_mode==FITNESS_MACRO_PR)
    {
        double precision=0.0, recall=0.0, macroF1=0.0;
        double weightedF1=0.0, gmean=0.0;
        getPrecisionAndRecall(precision,recall,macroF1,weightedF1,gmean);
        return 100.0*(1.0-(macro_pr_alpha*recall+(1.0-macro_pr_alpha)*precision));
    }
    else if(fitness_mode==FITNESS_MACRO_F1)
    {
        double precision=0.0;
        double recall=0.0;
        double macroF1=0.0;
        double weightedF1=0.0;
        double gmean=0.0;

        getPrecisionAndRecall(precision,recall,macroF1,weightedF1,gmean);
        return 100.0-100.0*macroF1;
    }
    else if(fitness_mode==FITNESS_WEIGHTED_F1)
    {
        double precision=0.0;
        double recall=0.0;
        double macroF1=0.0;
        double weightedF1=0.0;
        double gmean=0.0;

        getPrecisionAndRecall(precision,recall,macroF1,weightedF1,gmean);
        return 100.0-100.0*weightedF1;
    }
    return 0.0;
}

// ============================================================
// ERROR PER CLASS
// ============================================================

void ClassProgram::getErrorPerClass(vector<int> &genome,vector<double> &x)
{
    if(nclass<=1)return;

    outy.resize(trainy.size());
    pgenome.resize(genome.size()/(nclass-1));
    x.resize(nclass);
    for(int i=0;i<nclass;i++)x[i]=100.0;
    for(unsigned int i=0;i<outy.size();i++)
    {
        outy[i]=NAN_CLASS;
    }

    extern int wrapping;

    for(int i=0;i<nclass-1;i++)
    {
        for(unsigned int j=0;j<pgenome.size();j++)
        {
            pgenome[j]=genome[i*genome.size()/(nclass-1)+j];
        }

        int redo=0;
        string s =printRandomProgram(pgenome,redo);
        if(redo>=wrapping)
            return;

        pstring[i]=s;
    }

    for(int j=0;j<nclass-1;j++)
    {
        int d =program->Parse(pstring[j]);
        if(!d)return;

        for(unsigned int i=0;i<trainy.size();i++)
        {
            if(fabs(outy[i]-NAN_CLASS)>1e-5)
            {
                continue;
            }

            double v =program->Eval(trainx[i].data());

            if(program->EvalError())
            {
                return;
            }

            if(isnan(v) ||isinf(v))
            {
                return;
            }

            if(fabs(v-1.0)<1e-5)
            {
                outy[i]=vclass[j];
            }
        }
    }

    vector<int> fail;
    vector<int> belong;

    fail.resize(nclass,0);
    belong.resize(nclass,0);

    for(unsigned int i=0;i<trainy.size();i++)
    {
        if(fabs(outy[i]-NAN_CLASS)<1e-5)
        {
            outy[i]=vclass[nclass-1];
        }

        int pos =findMapper(trainy[i]);

        if(pos>=0 &&pos<nclass)
        {
            belong[pos]++;
            if(fabs(pos-outy[i])>1e-5)
            {
                fail[pos]++;
            }
        }
    }

    for(int i=0;i<nclass;i++)
    {
        if(belong[i]==0)
        {
            x[i]=0.0;
            continue;
        }
        x[i]=fail[i]*100.0/belong[i];
    }
}

// ============================================================
// FITNESS MODE
// ============================================================

void ClassProgram::setMacroPRAlpha(double alpha)
{
    if(alpha>=0.0 && alpha<=1.0) macro_pr_alpha=alpha;
}

void ClassProgram::setFitnessMode(int m)
{
    fitness_mode=m;
}

// Evaluate on a held-out subset of the outer training fold only.
// The external test fold is never used to tune weights.
double ClassProgram::validationMacroF1(vector<int> &genome, Dataset *validation)
{
    if(!validation || validation->count()==0) return -1.0;
    Dataset *saved = testSet;
    testSet = validation;
    quietTestReport = true;
    double error = getClassError(genome);
    quietTestReport = false;
    testSet = saved;
    if(error > 1e+7) return -1.0;
    vector<double> real, predicted;
    getOutputs(validation,real,predicted);
    double total=0.0;
    for(int c=0;c<nclass;c++) {
        double tp=0.0, fp=0.0, fn=0.0;
        for(size_t i=0;i<real.size();i++) {
            if((int)real[i]==c && (int)predicted[i]==c) tp++;
            else if((int)real[i]!=c && (int)predicted[i]==c) fp++;
            else if((int)real[i]==c && (int)predicted[i]!=c) fn++;
        }
        const double denom=2.0*tp+fp+fn;
        if(denom>0.0) total+=2.0*tp/denom;
    }
    return total/nclass;
}

// Geometric mean of per-class recalls on the held-out training subset.
// A missing class or zero recall gives score zero (conservative).
double ClassProgram::validationGmean(vector<int> &genome, Dataset *validation)
{
    if(!validation || validation->count()==0) return -1.0;
    Dataset *saved=testSet;
    testSet=validation;
    quietTestReport=true;
    double error=getClassError(genome);
    quietTestReport=false;
    testSet=saved;
    if(!std::isfinite(error) || error>1e+7) return -1.0;
    vector<double> real,predicted;
    getOutputs(validation,real,predicted);
    if(nclass<=0 || real.empty()) return -1.0;
    vector<double> support(nclass,0.0),correct(nclass,0.0);
    for(size_t i=0;i<real.size();++i) {
        const int c=(int)real[i];
        if(c<0 || c>=nclass) return -1.0;
        support[c]+=1.0;
        if((int)predicted[i]==c) correct[c]+=1.0;
    }
    double logSum=0.0;
    for(int c=0;c<nclass;++c) {
        if(support[c]==0.0 || correct[c]==0.0) return 0.0;
        logSum+=std::log(correct[c]/support[c]);
    }
    return std::exp(logSum/nclass);
}

// Minority-class F1 on the random validation holdout. The minority class
// is determined ONLY from the internal training partition, not validation.
// No samples are added, removed, reweighted, or stratified.
double ClassProgram::validationMinorityF1(vector<int> &genome, Dataset *validation)
{
    if(!validation || validation->count()==0 || nclass<2) return -1.0;
    std::vector<size_t> counts(nclass,0);
    for(size_t i=0;i<trainy.size();++i) {
        const int c=findMapper(trainy[i]);
        if(c>=0 && c<nclass) ++counts[c];
    }
    int minority=-1;
    for(int c=0;c<nclass;++c) {
        if(counts[c]==0) continue;
        if(minority<0 || counts[c]<counts[minority]) minority=c;
    }
    if(minority<0) return -1.0;
    Dataset *saved=testSet;
    testSet=validation;
    quietTestReport=true;
    const double error=getClassError(genome);
    quietTestReport=false;
    testSet=saved;
    if(!std::isfinite(error) || error>1e+7) return -1.0;
    std::vector<double> real,predicted;
    getOutputs(validation,real,predicted);
    double tp=0.0,fp=0.0,fn=0.0;
    for(size_t i=0;i<real.size();++i) {
        const bool actual=((int)real[i]==minority);
        const bool predictedMinority=((int)predicted[i]==minority);
        if(actual && predictedMinority) ++tp;
        else if(!actual && predictedMinority) ++fp;
        else if(actual && !predictedMinority) ++fn;
    }
    const double denom=2.0*tp+fp+fn;
    return denom>0.0 ? 2.0*tp/denom : 0.0;
}

// ============================================================
// FITNESS PERCENTAGES
// ============================================================

void ClassProgram::setFitnessPercentages(double p1,double p2,double p3)
{
    class_percent =p1>=0 &&p1<=1.0?p1:class_percent;
    average_percent =p2>=0 &&p2<=1.0?p2: average_percent;
    squared_percent =p3>=0 &&p3<=1.0?p3:squared_percent;
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
    Data testy =t->getAllYPoints();

    if(real.size()!=testy.size())
    {
        real.resize(testy.size());
        est.resize(testy.size());
    }
    /*
     * IMPORTANT:
     *
     * Class mapping comes from the TRAIN classes through
     * mapper/findMapper().
     *
     * Therefore TEST is allowed to contain fewer classes.
     */

    for(unsigned int i=0;i<testy.size();i++)
    {
        real[i]=findMapper(testy[i]);

        if(i<outy.size())
            est[i]=outy[i];
        else
            est[i]=0.0;
    }
}

// ============================================================
// GET TRAIN OUTPUTS
// ============================================================

void ClassProgram::getOutputs(vector<double> &real,vector<double> &est)
{
    getOutputs(trainSet,real,est);
}

// ============================================================
// DESTRUCTOR
// ============================================================

ClassProgram::~ClassProgram()
{
    delete program;
}
