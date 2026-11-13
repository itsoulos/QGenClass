#include <QCoreApplication>
# include <CORE/parameterlist.h>
# include <GE/population.h>
# include <GE/classprogram.h>
#include <CORE/featureconstruction.h>
#include <exception>
#include <map>
#include <algorithm>
#include <random>
#include <cmath>
# include <QDebug>
char    grammar_file[1024];
int     wrapping=2;
int     ok_to_print=0;

Dataset *trainSet    =NULL;
Dataset *testSet     =NULL;
ClassProgram *program=NULL;
Population *pop      =NULL;

ParameterList mainParams;


void 	printConfusionMatrix(vector<int> &genome,vector<double> &precision,vector<double> &recall)
{
    int i,j;
    vector<double> T;
    vector<double> O;
    // getOutputs() reads cached outy. Validation scoring overwrites that cache.
    // Recompute the TEST predictions for this exact genome before reading it.
    program->getClassError(genome);
    program->getOutputs(testSet,T,O);
    int N=T.size();
    int nclass=program->getClass();
    int **CM;
    printf("** CONFUSION MATRIX ** Number of classes: %d\n",nclass);
    CM=new int*[nclass];
    for(i=0;i<nclass;i++) CM[i]=new int[nclass];
    for(i=0;i<nclass;i++)
        for(j=0;j<nclass;j++) CM[i][j] = 0;

    for(i=0;i<N;i++) CM[(int)T[i]][(int)O[i]]++;

    precision.resize(nclass);
    recall.resize(nclass);

    for(i=0;i<nclass;i++)
    {
        double sum = 0.0;
        for(j=0;j<nclass;j++)
            sum+=CM[j][i];

        precision[i]=sum==0?0.0:CM[i][i]/sum;
        sum = 0.0;
        for(j=0;j<nclass;j++)
            sum+=CM[i][j];
        recall[i]=sum==0?0.0:CM[i][i]/sum;
    }

    for(i=0;i<nclass;i++)
    {
        for(j=0;j<nclass;j++)
        {
            printf("%4d ",CM[i][j]);
        }
        printf("\n");
        delete[] CM[i];
    }
    delete[] CM;
}

void setParams()
{
    QStringList fcModes; fcModes << "standard" << "feature_construction";
    mainParams.addParam(Parameter("mode",fcModes[0],fcModes,"Classifier mode"));
    mainParams.addParam(Parameter("features",5,1,100,"Number of constructed features"));
    mainParams.addParam(Parameter("train_file","","The trainfile used here"));
    mainParams.addParam(Parameter("test_file","","The test file used"));
    QStringList format;
    format<<"data"<<"csv"<<"arff";
    mainParams.addParam(Parameter("train_format",format[0],format,"Train file input format"));
    mainParams.addParam(Parameter("test_format",format[0],format,"Test file input format"));
    mainParams.addParam(Parameter("pop_count",500,10,10000,"Population count"));
    mainParams.addParam(Parameter("pop_gens",2000,10,10000,"Maximum number of generations"));
    mainParams.addParam(Parameter("pop_srate",0.1,0.0,1.0,"Selection rate"));
    mainParams.addParam(Parameter("pop_mrate",0.05,0.0,1.0,"Mutation rate"));
    mainParams.addParam(Parameter("pop_size",200,10,500,"The size of chromosomes"));
    QStringList local_list;
    local_list<<"none"<<"crossover"<<"mutate"
               <<"mutateWorst"<<"siman"
               <<"hill"<<"de"<<"gd"<<"adam"<<"targeted"
               <<"targetedWorst"<<"targetedBest"<<"constants";
    mainParams.addParam(
        Parameter("targeted_iterations",10,0,1000,"Targeted iterations")
        );

    mainParams.addParam(
        Parameter("targeted_stagnationlimit",20,0,100,"Targeted stagnation limit")
        );

    mainParams.addParam(Parameter(
        "targeted_burstiterations",200,0,1000,"Targeted burst iterations"));

    mainParams.addParam(Parameter("pop_lmethod",local_list[0],local_list,"Local search method"));


    QStringList crossList;
    crossList<<"standard"<<"targeted"<<"targetedWorst";
    mainParams.addParam(
        Parameter("cross_method",crossList[0],crossList,"Local crossover method")
        );
    mainParams.addParam(Parameter("pop_crossitems",10,0,1000,"Number of items participate in local crossover"));
    mainParams.addParam(Parameter("targeted_crossiterations",20,1,100,"Targeted cross iterations"));
    mainParams.addParam(Parameter("targeted_crosselitefraction",0.2,0.0,1.0,"Targeted cross elite fraction"));
    mainParams.addParam(Parameter("targeted_crossmaxblock",16,1,100,"Targeted max block"));

    mainParams.addParam(Parameter("pop_localgens",100,0,1000,"Number of generations before the application of local search"));
    mainParams.addParam(Parameter("pop_localitems",10,0,1000,"Number of chromosomes participate in localsearch"));

    QStringList fitness_list;
    fitness_list
        <<"class"<<"average"<<"squared"<<"mixed"<<"mean"
        <<"weightedF1"<<"macroF1"<<"macroPR"<<"adaptive_mixed"<<"adaptive_mixed_gmean"<<"adaptive_mixed_minority_f1"<<"minority_f1"<<"minority_fbeta"<<"balanced_sampling"<<"hybrid_sampling";

    mainParams.addParam(Parameter("pop_fitnessmethod",fitness_list[0],fitness_list,"Fitness calculation method"));

    mainParams.addParam(Parameter("sampling_interval",20,1,10000,"Generations per fixed balanced subset"));
    mainParams.addParam(Parameter("sampling_ratio",1,1,100,"Majority samples per minority sample"));
    mainParams.addParam(Parameter("sampling_lambda",0.75,0.0,1.0,"Weight of sampled balanced error in hybrid_sampling"));
    mainParams.addParam(Parameter("fbeta_beta",2.0,0.01,100.0,"Beta for minority_fbeta (higher favors recall)"));
    mainParams.addParam(Parameter("sampling_minority_n",0,0,1000000,"Minority subset size (0=all)"));
    mainParams.addParam(Parameter("adaptive_interval",20,1,1000,"Generations between adaptive weight updates"));
    mainParams.addParam(Parameter("adaptive_step",0.05,0.001,0.5,"Adaptive weight transfer step"));
    mainParams.addParam(Parameter("adaptive_validation",0.2,0.05,0.5,"Internal random validation fraction"));
    mainParams.addParam(Parameter("adaptive_hysteresis",0.02,0.0,1.0,"Minimum validation score improvement needed to change adaptive weights"));
    mainParams.addParam(Parameter("pop_macropr_alpha",0.5,0.0,1.0,"Recall weight for macroPR fitness (precision weight = 1-alpha)"));
    mainParams.addParam(Parameter("pop_classpercent",1.0,0.0,1.0,"The percentage of the class fitness method in mixed"));
    mainParams.addParam(Parameter("pop_averagepercent",0.0,0.0,1.0,"The percentage of the average fitness method in mixed"));
    mainParams.addParam(Parameter("pop_squaredpercent",0.0,0.0,1.0,"The percentage of the squared fitness method in mixed"));

    QStringList termMethod;
    termMethod<<"maxiters"<<"doublebox"<<"similarity";
    mainParams.addParam(Parameter("pop_termination",termMethod[0],termMethod,"Termination method for genetic algorithm"));
    QStringList yesno;
    yesno<<"no"<<"yes";
    mainParams.addParam(Parameter("enable_smote",yesno[0],yesno,"Enable / disable SMOTE in training set"));
    mainParams.addParam(Parameter("smote_k",5,2,100,"The k value for the smote procedure"));

    mainParams.addParam(Parameter("coutput_file","classifier.c","The name of the c output file"));
    mainParams.addParam(Parameter("pythonoutput_file","classifier.py","The name of the Python output file"));

    mainParams.addParam(Parameter("random_seed",1,0,1000,"Random Seed"));
    mainParams.addParam(Parameter("help","","Display help message"));

}
void printOption(Parameter param)
{
    qDebug().noquote()<<"Parameter name:           "<<param.getName();
    qDebug().noquote()<<"\tParameter default value:"<<param.getValue();
    qDebug().noquote()<<"\tParameter purpose:      "<<param.getHelp();
}

void done()
{
    if(trainSet!=NULL)  delete trainSet;
    if(testSet!=NULL)   delete testSet;
    if(program!=NULL)   delete program;
    if(pop!=NULL)       delete pop;
}

void shouldTerminate()
{
    done();
    qApp->exit(0);
    exit(0);
}
void    printHelp()
{
    qDebug().noquote()<<"MAIN PARAMETERS\n=================================================";
    for(int i=0;i<mainParams.countParameters();i++)
        printOption(mainParams.getParam(i));
    shouldTerminate();
}

void error(QString message)
{
    printf("Fatal error %s \n",message.toStdString().c_str());
    shouldTerminate();
}

void parseCmdLine(QStringList args)
{
    QString lastParam="";
    bool entered=false;
    for(int i=1;i<args.size();i++)
    {
        entered = true;
        if(args[i]=="--help") printHelp();
        QStringList lst = args[i].split("=");
        if(lst.size()<=1)
            error(QString("Fatal error %1 not an option").arg(args[i]));
        QString name = lst[0];
        QString value = lst[1];
        if(name.startsWith("--"))
            name = name.mid(2);
        if(value=="")
        {
            error(QString("Param %1 is empty.").arg(value));
        }
        bool foundParameter = false;
        //check in mainParams
        if(mainParams.contains(name))
        {
            mainParams.setParam(name,value);
            foundParameter=true;
        }
        if(!foundParameter)
            error(QString("Parameter %1 not found.").arg(name));
    }
    if(!entered)
        printHelp();

}
void init()
{
    setParams();
}

void run()
{
	srand(mainParams.getParam("random_seed").getValue().toInt());
    if(mainParams.getParam("train_file").getValue()=="")
        error(QString("Input train file not provided"));

    trainSet = new Dataset(
                mainParams.getParam("train_file").getValue(),
                mainParams.getParam("train_format").getValue()
                );
    QString hasSmote = mainParams.getParam("enable_smote").getValue();
    if(hasSmote == "yes")
    {
        int k = mainParams.getParam("smote_k").getValue().toInt();
        trainSet->makeSmote(k);
    }
    if(mainParams.getParam("test_file").getValue()!="")
    {
        testSet = new Dataset(
                    mainParams.getParam("test_file").getValue(),
                    mainParams.getParam("test_format").getValue()
                    );
    }
    if(mainParams.getParam("mode").getValue()=="feature_construction") {
        FeatureConstruction fc;
        try {
            fc.fit(*trainSet,
                   mainParams.getParam("features").getValue().toInt(),
                   mainParams.getParam("random_seed").getValue().toInt());
            fc.print();
            fc.transform(*trainSet);
            if(testSet) fc.transform(*testSet);
        } catch(const std::exception &e) {
            error(QString("Feature construction failed: %1").arg(e.what()));
        }
    }
    const QString adaptiveMethod=mainParams.getParam("pop_fitnessmethod").getValue();
    const bool adaptive = adaptiveMethod=="adaptive_mixed" || adaptiveMethod=="adaptive_mixed_gmean" || adaptiveMethod=="adaptive_mixed_minority_f1";
    const bool useGmean = adaptiveMethod=="adaptive_mixed_gmean";
    const bool useMinorityF1 = adaptiveMethod=="adaptive_mixed_minority_f1";
    Dataset *validationSet = NULL;
    if(adaptive) {
        Matrix allX=trainSet->getAllXpoint();
        Data allY=trainSet->getAllYPoints();
        Matrix fitX, valX;
        Data fitY, valY;
        std::mt19937 rng(mainParams.getParam("random_seed").getValue().toUInt());
        const double fraction=mainParams.getParam("adaptive_validation").getValue().toDouble();
        // Unstratified random holdout: no class-based sampling or balancing.
        std::vector<int> indices(allY.size());
        for(size_t j=0;j<indices.size();++j) indices[j]=(int)j;
        std::shuffle(indices.begin(),indices.end(),rng);
        size_t held=std::max((size_t)1,(size_t)std::round(indices.size()*fraction));
        if(held>=indices.size()) held=indices.size()-1;
        for(size_t j=0;j<indices.size();++j) {
            int index=indices[j];
            if(j<held) {valX.push_back(allX[index]);valY.push_back(allY[index]);}
            else {fitX.push_back(allX[index]);fitY.push_back(allY[index]);}
        }
        if(valY.empty()) error("adaptive_mixed needs at least two samples in a class for validation");
        trainSet->setData(fitX,fitY);
        validationSet=new Dataset();
        validationSet->setData(valX,valY);
        printf("ADAPTIVE_SPLIT training=%zu validation=%zu\n",fitY.size(),valY.size());
        // Diagnostics only: original labels and class counts, no resampling.
        std::map<double,size_t> trainCounts, valCounts;
        for(double y:fitY) ++trainCounts[y];
        for(double y:valY) ++valCounts[y];
        for(const auto &kv:trainCounts)
            printf("ADAPTIVE_CLASS_COUNTS label=%.10g training=%zu validation=%zu\n",
                   kv.first,kv.second,valCounts[kv.first]);
    }
    program = new ClassProgram(trainSet,testSet);
    pop=new Population(
                mainParams.getParam("pop_count").getValue().toInt(),
                mainParams.getParam("pop_size").getValue().toInt(),
                program
                );
    pop->setSelectionRate(mainParams.getParam("pop_srate").getValue().toDouble());
    pop->setMutationRate(mainParams.getParam("pop_mrate").getValue().toDouble());
    pop->setCrossItems(mainParams.getParam("pop_crossitems").getValue().toInt());
    pop->setLocalItems(mainParams.getParam("pop_localitems").getValue().toInt());
    pop->setLocalGens(mainParams.getParam("pop_localgens").getValue().toInt());
    pop->setTargetedIterations(
        mainParams.getParam("targeted_iterations").getValue().toInt());

    pop->setTargetedStagnationLimit(
        mainParams.getParam("targeted_stagnationlimit").getValue().toInt());

    pop->setTargetedBurstIterations(
        mainParams.getParam("targeted_burstiterations").getValue().toInt());

    pop->setCrossMethod(mainParams.getParam("cross_method").getValue().toStdString());
    pop->setTargetedCrossIterations(mainParams.getParam("targeted_crossiterations").getValue().toInt());
    pop->setTargetedCrossEliteFraction(mainParams.getParam("targeted_crosselitefraction").getValue().toDouble());
    pop->setTargetedCrossMaxBlock(mainParams.getParam("targeted_crossmaxblock").getValue().toInt());

    QString method = mainParams.getParam("pop_lmethod").getValue();
    pop->setLocalMethod(method.toStdString());
    QString fmethod = mainParams.getParam("pop_fitnessmethod").getValue();
    if(fmethod == "class")
	    program->setFitnessMode(FITNESS_CLASS);
    else
    if(fmethod == "average")
	    program->setFitnessMode(FITNESS_AVERAGE);
    else
    if(fmethod == "squared")
        program->setFitnessMode(FITNESS_SQUARED);
    else
        if(fmethod=="mixed" || fmethod=="adaptive_mixed" || fmethod=="adaptive_mixed_gmean" || fmethod=="adaptive_mixed_minority_f1")
        program->setFitnessMode(FITNESS_MIXED);
    else
        if(fmethod == "mean")
        program->setFitnessMode(FITNESS_MEAN);
    else
        if(fmethod == "weightedF1")
        program->setFitnessMode(FITNESS_WEIGHTED_F1);
    else
        if(fmethod == "macroF1")
        program->setFitnessMode(FITNESS_MACRO_F1);

    if(fmethod == "macroPR")
        program->setFitnessMode(FITNESS_MACRO_PR);
    if(fmethod == "minority_f1")
        program->setFitnessMode(FITNESS_MINORITY_F1);
    if(fmethod == "minority_fbeta") {
        program->setFitnessMode(FITNESS_MINORITY_FBETA);
        program->setMinorityFbeta(mainParams.getParam("fbeta_beta").getValue().toDouble());
    }
    if(fmethod == "balanced_sampling" || fmethod == "hybrid_sampling") {
        program->setFitnessMode(fmethod == "hybrid_sampling" ? FITNESS_HYBRID_SAMPLING : FITNESS_BALANCED_SAMPLING);
        program->setSamplingLambda(mainParams.getParam("sampling_lambda").getValue().toDouble());
        program->configureBalancedSampling(
            mainParams.getParam("sampling_interval").getValue().toInt(),
            mainParams.getParam("sampling_ratio").getValue().toInt(),
            mainParams.getParam("sampling_minority_n").getValue().toInt(),
            mainParams.getParam("random_seed").getValue().toUInt());
    }
    program->setMacroPRAlpha(mainParams.getParam("pop_macropr_alpha").getValue().toDouble());

    double p1 = mainParams.getParam("pop_classpercent").getValue().toDouble();
    double p2 = mainParams.getParam("pop_averagepercent").getValue().toDouble();
    double p3 = mainParams.getParam("pop_squaredpercent").getValue().toDouble();

    if(adaptive) {
        p1=p2=p3=1.0/3.0;
    }
    program->setFitnessPercentages(p1,p2,p3);
    const int adaptiveInterval=mainParams.getParam("adaptive_interval").getValue().toInt();
    const double adaptiveHysteresis=mainParams.getParam("adaptive_hysteresis").getValue().toDouble();
    bool adaptiveInitialized=false;
    double adaptiveBest=-1.0;
    auto updateAdaptive = [&](int gen) {
        // Evaluate the three independent training losses once per genome.
        // A weighted sum then needs no further rule evaluation.
        struct Losses { double v[3]; bool valid; };
        std::vector<Losses> losses(pop->getCount());
        const int modes[3]={FITNESS_CLASS,FITNESS_AVERAGE,FITNESS_SQUARED};
        for(int k=0;k<pop->getCount();++k) {
            std::vector<int> g=pop->getGenome(k);
            losses[k].valid=true;
            for(int j=0;j<3;++j) {
                program->setFitnessMode(modes[j]);
                losses[k].v[j]=program->fitness(g);
                if(!std::isfinite(losses[k].v[j]) || losses[k].v[j]>=1e+7)
                    losses[k].valid=false;
            }
        }
        program->setFitnessMode(FITNESS_MIXED);
        // Validation depends on the selected genome, not on its weights.
        // Cache its validation score when it is selected by multiple triples.
        std::vector<double> validationScores(pop->getCount(),-1.0);
        double bestScore=-1.0;
        double best[3]={p1,p2,p3};
        int bestCandidate=-1;
        int bestIndex=-1;
        double bestTraining=1e+100;
        int candidate=0;
        // Integer grid on the simplex: 66 triples at increments of 0.1.
        for(int ic=0;ic<=10;++ic) for(int ia=0;ia<=10-ic;++ia) {
            const int is=10-ic-ia;
            const double w[3]={ic/10.0,ia/10.0,is/10.0};
            int selectedIndex=-1;
            double minFitness=1e+100;
            for(int k=0;k<pop->getCount();++k) {
                if(!losses[k].valid) continue;
                const double f=w[0]*losses[k].v[0]
                              +w[1]*losses[k].v[1]
                              +w[2]*losses[k].v[2];
                if(f<minFitness) {minFitness=f;selectedIndex=k;}
            }
            if(selectedIndex<0) {
                printf("ADAPTIVE_CANDIDATE generation=%d candidate=%d invalid=1\n",gen,candidate++);
                continue;
            }
            if(validationScores[selectedIndex]<0.0) {
                std::vector<int> g=pop->getGenome(selectedIndex);
                validationScores[selectedIndex]=useMinorityF1 ? program->validationMinorityF1(g,validationSet) : (useGmean ? program->validationGmean(g,validationSet) : program->validationMacroF1(g,validationSet));
            }
            const double score=validationScores[selectedIndex];
            printf("ADAPTIVE_CANDIDATE generation=%d candidate=%d class=%.6f average=%.6f squared=%.6f chromosome=%d training_fitness=%.10f validation_score=%.10f metric=%s\n",
                   gen,candidate,w[0],w[1],w[2],selectedIndex,minFitness,score,useMinorityF1?"minority_f1":(useGmean?"gmean":"macro_f1"));
            // Prefer the incumbent on ties, then lower training loss.
            const bool incumbent=std::fabs(w[0]-p1)<1e-9 &&
                std::fabs(w[1]-p2)<1e-9 && std::fabs(w[2]-p3)<1e-9;
            const bool bestIncumbent=std::fabs(best[0]-p1)<1e-9 &&
                std::fabs(best[1]-p2)<1e-9 && std::fabs(best[2]-p3)<1e-9;
            if(std::isfinite(score) &&
               (score>bestScore+1e-12 ||
                (std::fabs(score-bestScore)<=1e-12 &&
                 ((incumbent && !bestIncumbent) ||
                  (incumbent==bestIncumbent && minFitness<bestTraining-1e-12))))) {
                bestScore=score;bestCandidate=candidate;bestIndex=selectedIndex;
                bestTraining=minFitness;
                for(int j=0;j<3;++j) best[j]=w[j];
            }
            ++candidate;
        }
        // Evaluate the incumbent weights on THIS generation's population.
        // Comparing with the previous generation's score would not be fair.
        int incumbentIndex=-1;
        double incumbentTraining=1e+100;
        for(int k=0;k<pop->getCount();++k) {
            if(!losses[k].valid) continue;
            const double f=p1*losses[k].v[0]+p2*losses[k].v[1]+p3*losses[k].v[2];
            if(f<incumbentTraining) { incumbentTraining=f; incumbentIndex=k; }
        }
        double incumbentScore=-1.0;
        if(incumbentIndex>=0) {
            if(validationScores[incumbentIndex]<0.0) {
                std::vector<int> g=pop->getGenome(incumbentIndex);
                validationScores[incumbentIndex]=useMinorityF1 ? program->validationMinorityF1(g,validationSet) : (useGmean ? program->validationGmean(g,validationSet) : program->validationMacroF1(g,validationSet));
            }
            incumbentScore=validationScores[incumbentIndex];
        }
        const bool proposedChange=std::fabs(p1-best[0])>1e-9 ||
            std::fabs(p2-best[1])>1e-9 || std::fabs(p3-best[2])>1e-9;
        const double proposedScore=bestScore;
        const bool keepIncumbent=adaptiveInitialized && proposedChange &&
            std::isfinite(incumbentScore) && incumbentScore>=0.0 &&
            bestScore<incumbentScore+adaptiveHysteresis-1e-12;
        if(keepIncumbent) {
            best[0]=p1; best[1]=p2; best[2]=p3;
            bestScore=incumbentScore;
            bestIndex=incumbentIndex;
            bestCandidate=-1; // incumbent retained by hysteresis
        }
        const bool changed=std::fabs(p1-best[0])>1e-9 ||
            std::fabs(p2-best[1])>1e-9 || std::fabs(p3-best[2])>1e-9;
        if(bestCandidate>=0) {p1=best[0];p2=best[1];p3=best[2];}
        adaptiveBest=bestScore;
        adaptiveInitialized=true;
        printf("ADAPTIVE_HYSTERESIS generation=%d threshold=%.6f incumbent_score=%.10f candidate_score=%.10f kept=%d\n",
               gen,adaptiveHysteresis,incumbentScore,proposedScore,keepIncumbent?1:0);
        program->setFitnessPercentages(p1,p2,p3);
        printf("ADAPTIVE_MIXED generation=%d class=%.6f average=%.6f squared=%.6f validation_score=%.6f metric=%s changed=%d selected_candidate=%d chromosome=%d grid_size=66\n",
               gen,p1,p2,p3,adaptiveBest,useMinorityF1?"minority_f1":(useGmean?"gmean":"macro_f1"),changed?1:0,bestCandidate,bestIndex);
        fflush(stdout);
        std::vector<int> selectedBeforeRefresh;
        if(useMinorityF1 && bestIndex>=0)
            selectedBeforeRefresh=pop->getGenome(bestIndex);
        pop->refreshFitness();
        // Compare the chromosome chosen on validation against the chromosome
        // actually returned by the population after its fitness refresh.
        if(useMinorityF1 && bestIndex>=0) {
            std::vector<int> reported=pop->getBestGenome();
            const double selectedVal=program->validationMinorityF1(selectedBeforeRefresh,validationSet);
            const double reportedVal=program->validationMinorityF1(reported,validationSet);
            printf("ADAPTIVE_DIAGNOSTIC generation=%d selected_index=%d selected_validation_f1=%.10f reported_validation_f1=%.10f same_genome=%d\n",
                   gen,bestIndex,selectedVal,reportedVal,selectedBeforeRefresh==reported?1:0);
            fflush(stdout);
        }
    };
    if(adaptive) updateAdaptive(0);

    int gens = mainParams.getParam("pop_gens").getValue().toInt();
    vector<int> genome;
    for(int i=1;i<=gens;i++)
    {
        pop->nextGeneration();
        if(adaptive && i%adaptiveInterval==0) updateAdaptive(i);
        //print
        genome=pop->getBestGenome();
        pop->evaluateBestFitness();
        double bestf=pop->getBestFitness();
        string s = program->printF(genome);
	if(fabs(bestf)>1e+7) continue;
        if(testSet==NULL)
            printf("GENERATION=%5d FITNESS=%10.4lg%%\nPROGRAMS=\n%s",
                i,bestf,s.c_str());
            else
            printf("GENERATION=%5d FITNESS=%10.4lg%% TEST_ERROR=%10.4lg%%\nPROGRAMS=\n%s",
               i,bestf,-program->getClassError(genome),s.c_str());

	    if(fabs(bestf)<1e-6) break;

    if(adaptive && useMinorityF1 && validationSet) {
        std::vector<int> finalGenome=pop->getBestGenome();
        const double finalValidation=program->validationMinorityF1(finalGenome,validationSet);
        printf("ADAPTIVE_FINAL_DIAGNOSTIC generation=%d final_genome_validation_minority_f1=%.10f last_selected_validation_score=%.10f\n",
               i,finalValidation,adaptiveBest);
        fflush(stdout);
    }
    vector<double> precision;
    vector<double> recall;
    printConfusionMatrix(genome,precision,recall);
    // Macro metrics over the fixed class set defined by training data.
    // Undefined per-class precision/recall is treated as zero.
    double avg_precision = 0.0, avg_recall = 0.0, avg_f1score = 0.0;
    int nclass=program->getClass();
    for(int i=0;i<nclass;i++)
    {
        const double p=precision[i];
        const double r=recall[i];
        avg_precision+=p;
        avg_recall+=r;
        avg_f1score+=(p+r>0.0)?2.0*p*r/(p+r):0.0;
    }
    if(nclass>0)
    {
        avg_precision/=nclass;
        avg_recall/=nclass;
        avg_f1score/=nclass;
    }
    printf("CLASS_ERROR: %15.10lf PRECISION: %15.10lf RECALL: %15.10lf F1SCORE: %15.10lf\n",
           -program->getClassError(genome),
           avg_precision,avg_recall,avg_f1score);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    setlocale(LC_ALL,"C");
    init();
    parseCmdLine(app.arguments());
    run();
    done();
    return 0;
}
