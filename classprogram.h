# ifndef __CLASSPROGRAM__H
# include <program.h>
# include <cprogram.h>
# include <dataset.h>
# include <vector>
# include <string>
using namespace std;

# define FITNESS_CLASS 	0x1
# define FITNESS_AVERAGE 0x2
# define FITNESS_SQUARED 0x4
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
	public:
        ClassProgram(Dataset *tr,Dataset *tt);
	void setFitnessMode(int m);
		string	printF(vector<int> &genome);
        void    printPython(vector<int> &genome, std::string outname = "classifier.py");
        void    printC(vector<int> &genome, std::string outname = "classifier.h");
        int     findMapper(double x);
		virtual double 	fitness(vector<int> &genome);
        double	getClassError(vector<int> &genome);
		void	getOutputs(vector<double> &real,vector<double> &est);
        int     getClass() const;
		~ClassProgram();
};
# define __CLASSPROGRAM__H
# endif
