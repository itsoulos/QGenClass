# ifndef __CLASSPROGRAM__H
# include <GE/program.h>
# include <GE/cprogram.h>
# include <CORE/dataset.h>
# include <vector>
# include <string>
using namespace std;

# define FITNESS_CLASS 	 0x1
# define FITNESS_AVERAGE 0x2
# define FITNESS_SQUARED 0x4
# define FITNESS_MIXED   0x8
# define FITNESS_MEAN    0xF
typedef vector<double> Data;
class ClassProgram	:public Program
{
	private:
        Dataset *trainSet;
        Dataset *testSet;
        Matrix trainx;
        Data   trainy;
		vector<double> vclass;
		vector<string> pstring;
		vector<int>    pgenome;
		Cprogram *program;
		vector<double> mapper;
		int dimension,nclass;
        Data outy;
        int fitness_mode = FITNESS_CLASS;
        double class_percent=1.0,average_percent=0.0,squared_percent=0.0;
        Data realCached,estCached;
	public:
        ClassProgram(Dataset *tr,Dataset *tt);
        void    setFitnessMode(int m);
        void    setFitnessPercentages(double p1,double p2,double p3);
		string	printF(vector<int> &genome);
        void    printPython(vector<int> &genome, std::string outname = "classifier.py");
        void    printC(vector<int> &genome, std::string outname = "classifier.h");
        int     findMapper(double x);
		virtual double 	fitness(vector<int> &genome);
        double	getClassError(vector<int> &genome);
        void 	getOutputs(Dataset *t,vector<double> &real,vector<double> &est);
        void 	getOutputs(vector<double> &real,vector<double> &est);
        int     getClass() const;
        void    getPrecisionAndRecall(double &precision,double &recall);
        void    getPrecisionAndRecall(Dataset *t,double &precision,double &recall);

		~ClassProgram();
};
# define __CLASSPROGRAM__H
# endif
