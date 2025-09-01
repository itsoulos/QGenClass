#include <QCoreApplication>
# include <parameterlist.h>
# include <population.h>
# include <classprogram.h>
# include <QDebug>
char    grammar_file[1024];
int     wrapping=2;
int     ok_to_print=0;

Dataset *trainSet    =NULL;
Dataset *testSet     =NULL;
ClassProgram *program=NULL;
Population *pop      =NULL;

ParameterList mainParams;

void setParams()
{
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
    mainParams.addParam(Parameter("pop_lrate",0.0,0.0,1.0,"Local search rate"));
    mainParams.addParam(Parameter("pop_size",200,10,500,"The size of chromosomes"));
    QStringList local_list;
    local_list<<"none"<<"crossover"<<"mutate"<<"siman";
    mainParams.addParam(Parameter("pop_lmethod",local_list[0],local_list,"Local search method"));

    QStringList fitness_list;
    fitness_list<<"class"<<"average"<<"squared";
    mainParams.addParam(Parameter("pop_fitnessmethod",fitness_list[0],fitness_list,"Fitness calculation method"));
    QStringList termMethod;
    termMethod<<"maxiters"<<"doublebox"<<"similarity";
    mainParams.addParam(Parameter("pop_termination",termMethod[0],termMethod,"Termination method for genetic algorithm"));
    QStringList yesno;
    yesno<<"no"<<"yes";
    mainParams.addParam(Parameter("enable_smote",yesno[0],yesno,"Enable / disable SMOTE in training set"));

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
    if(mainParams.getParam("train_file").getValue()=="")
        error(QString("Input train file not provided"));

    trainSet = new Dataset(
                mainParams.getParam("train_file").getValue(),
                mainParams.getParam("train_format").getValue()
                );
    if(mainParams.getParam("test_file").getValue()!="")
    {
        testSet = new Dataset(
                    mainParams.getParam("test_file").getValue(),
                    mainParams.getParam("test_format").getValue()
                    );
    }
    program = new ClassProgram(trainSet,testSet);
    pop=new Population(
                mainParams.getParam("pop_count").getValue().toInt(),
                mainParams.getParam("pop_size").getValue().toInt(),
                program,
                mainParams.getParam("random_seed").getValue().toInt()
                );
    pop->setSelectionRate(mainParams.getParam("pop_srate").getValue().toDouble());
    pop->setMutationRate(mainParams.getParam("pop_mrate").getValue().toDouble());
    pop->setLocalSearchRate(mainParams.getParam("pop_lrate").getValue().toDouble());
    QString method = mainParams.getParam("pop_lmethod").getValue();
    if(method == "none")
        pop->setLocalMethod(GELOCAL_NONE);
    else
    if(method == "crossover")
        pop->setLocalMethod(GELOCAL_CROSSOVER);
    else
    if(method == "mutate")
        pop->setLocalMethod(GELOCAL_MUTATE);
    else
    if(method == "siman")
        pop->setLocalMethod(GELOCAL_SIMAN);
    QString fmethod = mainParams.getParam("pop_fitnessmethod").getValue();
    if(fmethod == "class")
	    program->setFitnessMode(FITNESS_CLASS);
    else
    if(fmethod == "average")
	    program->setFitnessMode(FITNESS_AVERAGE);
    else
	program->setFitnessMode(FITNESS_SQUARED);    

    int gens = mainParams.getParam("pop_gens").getValue().toInt();
    vector<int> genome;
    for(int i=1;i<=gens;i++)
    {
        pop->nextGeneration();
        //print
        genome=pop->getBestGenome();
        pop->evaluateBestFitness();
        double bestf=pop->getBestFitness();
        string s = program->printF(genome);
        if(testSet==NULL)
            printf("GENERATION=%5d FITNESS=%10.4lg%%\nPROGRAMS=\n%s",
                i,bestf,s.c_str());
            else
            printf("GENERATION=%5d FITNESS=%10.4lg%% TEST_ERROR=%10.4lg%%\nPROGRAMS=\n%s",
               i,bestf,program->getClassError(genome),s.c_str());

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
