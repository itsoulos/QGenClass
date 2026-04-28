# ifndef __POPULATION__H
# include <GE/program.h>
# include <random>

# define GELOCAL_NONE       0x0
# define GELOCAL_CROSSOVER  0x1
# define GELOCAL_MUTATE     0x2
# define GELOCAL_BFGS       0x4
# define GELOCAL_SIMAN      0x8
# define GELOCAL_DE         0x16
# define GELOCAL_HILL       0x32
# define GELOCAL_GD         0x64
# define GELOCAL_ADAM       0x128

/* The Population class holds the current population. */
/* The mutation, selection and crossover operators are defined here */
class Population
{
	private:
		int	**children;
		int	**trialx;
		int	**genome;
		int	*valid;
		double *fitness_array;
		double	mutation_rate,selection_rate;
		int	genome_count;
		int	genome_size;
		int	generation;
		Program	*program;

        void    	select();
        void    	crossover();
        void    	mutate();
        void    	calcFitnessArray();
        void    	replaceWorst();
		int	elitism;
        string localMethod="none";
        void    	localSearch(int gpos);
	public:
		Population(int gcount,int gsize,Program *p);
		double 	fitness(vector<int> &g);
		void	setElitism(int s);
        void    setLocalMethod(string s);
        int     getGeneration() const;
        int     getCount() const;
        int     getSize() const;
        void    nextGeneration();
        void    	setMutationRate(double r);
        void    	setSelectionRate(double r);
		double	getSelectionRate() const;
		double	getMutationRate() const;
		double	getBestFitness() const;
		double	evaluateBestFitness();
		vector<int> getBestGenome() const;
        void	 reset();
        vector<int> discreteGradient( vector<int>& x);
        vector<int> discreteStep(vector<int>& x,vector<int>& grad);
        void integerLocalSearch(vector<int> &x,int maxSteps = 20);
        vector<int> integerAdam(
            vector<int> x,
            int steps = 20,
            double alpha = 0.5,
            double beta1 = 0.9,
            double beta2 = 0.999,
            double eps = 1e-8
            ) ;
        vector<int> neighbor(const vector<int>& x, int stepSize = 1);
        vector<int> simulatedAnnealing(
            vector<int> &x,
            double T0 = 100.0,
            double Tmin = 1e-3,
            double alpha = 0.95,
            int iterPerTemp = 20
            );
	void crossItem(int pos);
	void mutateItem(int pos);
		~Population();
		
};
# define __POPULATION__H
# endif
