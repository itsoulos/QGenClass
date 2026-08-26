# ifndef __CLASSPROGRAM__H
# include <GE/program.h>
# include <GE/cprogram.h>
# include <CORE/dataset.h>
# include <vector>
# include <string>
using namespace std;

# define FITNESS_CLASS 	      1
# define FITNESS_AVERAGE      2
# define FITNESS_SQUARED      4
# define FITNESS_MIXED        8
# define FITNESS_MEAN        16
# define FITNESS_MACRO_F1    32
# define FITNESS_WEIGHTED_F1 64
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
        void    getPrecisionAndRecall(double &precision,double &recall,
                                   double &macroF1, double &weightedF1,
                                   double &gmean);
        void    getPrecisionAndRecall(Dataset *t,
                                   double &precision,double &recall,
                                   double &macroF1, double &weightedF1,
                                   double &gmean);
        /**
        * @brief getErrorPerClass returns the error per
        * class for chromosome g.
        * @param g The input chromosome
        * @param x The output vector containing
        *  the error per class.
        */
        void getErrorPerClass(vector<int> &g,
                              vector<double> &x);
        int getDimension() const;
		~ClassProgram();
};
# define __CLASSPROGRAM__H
# endif
