# include <GE/population.h>
# include <stdlib.h>
# include <string.h>
# include <math.h>
# include <GE/classprogram.h>
# define MAX_RULE	256

mt19937 gen(random_device{}());

double randDouble(double a, double b) {
    uniform_real_distribution<> dist(a, b);
    return dist(gen);
}

int randInt(int a, int b) {
    uniform_int_distribution<> dist(a, b);
    return dist(gen);
}
vector<int> Population::neighbor(const vector<int>& x, int stepSize ) {
    vector<int> y = x;

    int i = randInt(0, x.size() - 1);

    // αλλαγή σε μία διάσταση
    y[i] += randInt(-stepSize, stepSize);
    if(y[i]<0) y[i]=0;

    return y;
}

vector<int> Population::simulatedAnnealing(
    vector<int> &x,
    double T0,
    double Tmin,
    double alpha,
    int iterPerTemp
    ) {
    double T = T0;

    vector<int> best = x;
    double bestVal = fitness(x);

    while (T > Tmin) {

        for (int k = 0; k < iterPerTemp; k++) {

            vector<int> y = neighbor(x);

            double fx = (fitness(x));
            double fy = (fitness(y));

            double delta = fabs(fy) - fabs(fx);

            if (delta < 0) {
                // καλύτερη λύση
                x = y;
            } else {
                // πιθανότητα αποδοχής
                double p = exp(-delta / T);

                if (randDouble(0,1) < p)
                    x = y;
            }

            // update best
            double val = fitness(x);
            if (fabs(val) < fabs(bestVal)) {
                best = x;
                bestVal = val;
            }
        }
        printf("T=%20.10lf BEST=%20.10lf\n",T,bestVal);
        // cooling
        T *= alpha;
    }

    return best;
}
/* Population constructor */
/* Input: genome count , genome size, pointer to Program instance */
Population::Population(int gcount,int gsize,Program *p)
{
	elitism=1;
	selection_rate = 0.1;
	mutation_rate  = 0.1;
	genome_count   = gcount;
	genome_size    = gsize;
	generation     = 0;
	program        = p;

	/* Create the population and based on genome count and size */
	/* Initialize the genomes to random */
	double f;
	genome=new int*[genome_count];
	children=new int*[genome_count];
	vector<int> g;
	g.resize(genome_size);
	for(int i=0;i<genome_count;i++)
	{
		genome[i]=new int[genome_size];
		children[i]=new int[genome_size];
			for(int j=0;j<genome_size;j++)
				g[j]=genome[i][j]=rand()%MAX_RULE;
	}
	fitness_array=new double[genome_count];
}

/* Reinitialize the population to random */
void    	Population::reset()
{
	generation = 0;
	for(int i=0;i<genome_count;i++)
		for(int j=0;j<genome_size;j++)
				genome[i][j]=rand()%MAX_RULE;
	for(int i=0;i<genome_count;i++)
			fitness_array[i]=-1e+100;
}

/* Return the fitness of a genome */
double 	Population::fitness(vector<int> &g)
{
	return -program->fitness(g);
}

/* The selection of the chromosomes according to the fitness value is performed */
void	 Population::select()
{
	int itemp[genome_size];
	for(int i=0;i<genome_count;i++)
	{
		for(int j=0;j<genome_count-1;j++)
		{
			if(fitness_array[j+1]>fitness_array[j])
			{
				double dtemp;
				dtemp=fitness_array[j];
				fitness_array[j]=fitness_array[j+1];
				fitness_array[j+1]=dtemp;
				
				memcpy(itemp,genome[j],genome_size*sizeof(int));
				memcpy(genome[j],genome[j+1],genome_size*sizeof(int));
				memcpy(genome[j+1],itemp,genome_size*sizeof(int));
			}
		}
	}
}

/* Crossover operation: based on tournament selection */
/* Select the tournament_size based on the genome count : */
/*     (if genome_count > 100 ) tournament_size = 10   else   tournament_size = 4 */
/* Select 2 chromosomes based on the tournament size and cross them over based on the crossover probability */
/* There is 1 crossover point and it is random */
void        Population::crossover()
{
        int parent[2];
        int nchildren=(int)((1.0 - selection_rate) * genome_count);
	if(!(nchildren%2==0)) nchildren++;
        const int tournament_size =(genome_count<=100)?4:20;
        int count_children=0;
        while(1)
        {
		// The two parents are selected here according to the tournament selection procedure
                for(int i=0;i<2;i++)
                {
                        double max_fitness=-1e+10;
                        int    max_index=-1;
			int r;
			// Select the best parents of  the candidates 
                        for(int j=0;j<tournament_size;j++)
                        {
				r=rand() % (genome_count);
                                if(j==0 || fitness_array[r]>max_fitness)
                                {
                                        max_index=r;
                                        max_fitness=fitness_array[r];
                                }
                        }
                        parent[i]=max_index;
			
                }
		int pt1,pt2;
		// The one-point crossover is performed here (the point is pt1)
		pt1=rand() % genome_size;
		memcpy(children[count_children],
				genome[parent[0]],pt1 * sizeof(int));
		memcpy(&children[count_children][pt1],
			&genome[parent[1]][pt1],(genome_size-pt1)*sizeof(int));
		memcpy(children[count_children+1],
				genome[parent[1]],pt1 * sizeof(int));
		memcpy(&children[count_children+1][pt1],
			&genome[parent[0]][pt1],(genome_size-pt1)*sizeof(int));
		count_children+=2;
		if(count_children>=nchildren) break;
	}
	
	vector<int> g;
	g.resize(genome_size);
	
	for(int i=0;i<nchildren;i++)
	{
		//for(int j=0;j<g.size();j++) g[j]=children[i][j];
		//double f=fitness(g);
		//if(fabs(f)<fabs(fitness_array[genome_count-i-1]))
		memcpy(genome[genome_count-i-1],
				children[i],genome_size * sizeof(int));
	}
}

void        Population::setElitism(int s)
{
	elitism = s;
}

/* Mutate the current population */
/* Standard mutation algorithm: mutate all chromosomes in the population based on the mutation probability */
void    	Population::mutate()
{
	int start = elitism * (int)(genome_count*selection_rate);
	start = elitism;
	start = 1;
	for(int i=start;i<genome_count;i++)
	{
		for(int j=0;j<genome_size;j++)
		{
			double r=rand()*1.0/RAND_MAX;
			if(r<mutation_rate)
			{
				genome[i][j]=rand() % MAX_RULE;
			}
		}
	}
}

/* Evaluate the fitness for all chromosomes in the current population */
void    	Population::calcFitnessArray()
{
	vector<int> g;
	g.resize(genome_size);

	double dmin=1e+100;
	int icount=0;
	for(int i=0;i<genome_count;i++)
	{
		for(int j=0;j<genome_size;j++) g[j]=genome[i][j];	
		fitness_array[i]=fitness(g);
        //if(rand()*1.0/RAND_MAX<=0.005)
        //	localSearch(i);

        if(fabs(fitness_array[i])<dmin)
		{
			dmin=fabs(fitness_array[i]);
		}
		if(fabs(fitness_array[i])>=1e+100) icount++;	
		
        if((i+1)%50==0)
		{
			printf(" %d:%.5lg ",i+1,dmin);
			fflush(stdout);
        }
		
	}
}

/* Return the current generation */
int	Population::getGeneration() const
{
	return generation;
}

/* Return the genome count */
int	Population::getCount() const
{
	return genome_count;
}

/* Return the size of the population */
int	Population::getSize() const
{
	return genome_size;
}

void    Population::setCrossItems(int g)
{
    if(g>=0)
        crossitems=g;
}

void    Population::setLocalItems(int g)
{
    if(g>=0)
        localitems=g;
}

void    Population::setLocalGens(int g)
{
    if(g>=0)
        localgens=g;
}

/* Evolve the next generation */
void        Population::nextGeneration()
{
	calcFitnessArray();
	select();
        if((generation+1)%localgens==0)
    	{
            for(int i=0;i<localitems;i++)
                localSearch(i==0?0:rand()%genome_count);
            select();
    	}
    for(int i=0;i<crossitems;i++)
		crossItem(i==0?0:rand() % genome_count);
	select();
	crossover();
	if(generation) mutate();
	++generation;
}

void    	Population::replaceWorst()
{
	vector<int> xtrial;
	xtrial.resize(genome_size);
	int randpos;
	randpos=rand() % genome_count;
	for(int i=0;i<genome_size;i++)
	{
		double gamma;
        gamma=-0.5+2.0*rand()*1.0/RAND_MAX;
		xtrial[i]=(int)(fabs((1.0+gamma)*genome[0][i]-gamma*genome[randpos][i]));
	}
	double ftrial = fitness(xtrial);
    if(fabs(ftrial)<fabs(fitness_array[genome_count-1]))
	{
		for(int i=0;i<genome_size;i++)
		genome[genome_count-1][i]=xtrial[i];
		fitness_array[genome_count-1]=ftrial;
	}
}

/* Set the mutation rate */
void        Population::setMutationRate(double r)
{
	if(r>=0 && r<=1) mutation_rate = r;
}

/* Set the selection rate */
void    	Population::setSelectionRate(double r)
{
	if(r>=0 && r<=1) selection_rate = r;
}

/* Return the selection rate */
double	Population::getSelectionRate() const
{
	return selection_rate;
}

/* Return the mutation rate */
double	Population::getMutationRate() const
{
	return mutation_rate;
}

/* Return the best fitness for this population */
double	Population::getBestFitness() const
{
	return  fitness_array[0];
}

/* Return the best chromosome */
vector<int> Population::getBestGenome() const
{
	vector<int> g;g.resize(genome_size);
	for(int i=0;i<genome_size;i++) g[i]=genome[0][i];
	return g;
}

vector<int> Population::discreteGradient(vector<int>& x)
{
    int n = x.size();
    vector<int> grad(n);

    double fx = fitness(x);

    for (int i = 0; i < n; i++) {

        vector<int> x_plus = x;
        vector<int> x_minus = x;

        x_plus[i] += 1;
        x_minus[i] -= 1;
        if(x_minus[i]<0) x_minus[i]=0;

        double f_plus = fitness(x_plus);
        double f_minus = fitness(x_minus);

        if (fabs(f_plus) < fabs(fx))
            grad[i] = +1;
        else if (fabs(f_minus) < fabs(fx))
            grad[i] = -1;
        else
            grad[i] = 0;
    }
    return grad;
}

vector<int> Population::discreteStep(vector<int>& x,vector<int>& grad)
{
    vector<int> res = x;

    for (int i = 0; i < (int)x.size(); i++)
    {
        res[i] += grad[i];
    }

    return res;
}

void Population::integerLocalSearch(vector<int> &x,int maxSteps)
{
    double bestVal = fitness(x);
    int stepSize = 100;
    for (int step = 0; step < maxSteps; step++) {

        vector<int> grad = discreteGradient(x);

        vector<int> candidate = x;

        for (int i = 0; i < x.size(); i++)
        {
            candidate[i] += grad[i] * stepSize;
            if(candidate[i]<0) candidate[i]=0;
        }

        double val = fitness(candidate);

        if (fabs(val) < fabs(bestVal)) {
            x = candidate;
            bestVal = val;
        } else {
            stepSize = max(1, stepSize / 2); // learning rate decay
        }
        printf("GD[%4d]=%20.10lg\n",step,bestVal);
    }


}


vector<int> Population::integerAdam(
    vector<int> x,
    int steps,
    double alpha,
    double beta1,
    double beta2,
    double eps
    ) {
    int n = x.size();

    vector<double> m(n, 0.0);
    vector<double> v(n, 0.0);

    double bestVal = fitness(x);

    for (int t = 1; t <= steps; t++) {

        vector<int> g_int = discreteGradient(x);
printf("ADAM[%d] TRY: %20.10lf\n",t,bestVal);
        // convert gradient to double
        vector<double> g(n);
        for (int i = 0; i < n; i++)
            g[i] = (double)g_int[i];

        // update moments
        for (int i = 0; i < n; i++) {
            m[i] = beta1 * m[i] + (1 - beta1) * g[i];
            v[i] = beta2 * v[i] + (1 - beta2) * g[i] * g[i];
        }

        // bias correction
        vector<double> m_hat(n), v_hat(n);
        for (int i = 0; i < n; i++) {
            m_hat[i] = m[i] / (1 - pow(beta1, t));
            v_hat[i] = v[i] / (1 - pow(beta2, t));
        }

        // candidate update
        vector<int> candidate = x;

        for (int i = 0; i < n; i++) {

            double step = alpha * m_hat[i] / (sqrt(v_hat[i]) + eps);
            double p = fabs(step);

            if (rand()*1.0/RAND_MAX < p) {
                candidate[i] += (step > 0 ? 1 : -1);
            }

            if(candidate[i]<0) candidate[i]=0;
        }

        double val = fitness(candidate);

        if (fabs(val) < fabs(bestVal)) {
            x = candidate;
            bestVal = val;
        }
        else {
            alpha *= 0.7; // decay learning rate
        }
    }

    return x;
}

void    Population::setLocalMethod(string s)
{
    localMethod = s;
}

void	Population::crossItem(int pos)
{
	vector<int> g;
	g.resize(genome_size);
	for(int i=0;i<genome_size;i++) g[i]=genome[pos][i];
	printf("CROSS[%d]=",pos);
	fflush(stdout);
	for(int iters=1;iters<=100;iters++)
	{
            int gpos,cutpoint;
        again:
            gpos=rand() % genome_count;
            cutpoint=rand() % genome_size;
		for(int j=0;j<cutpoint;j++) g[j]=genome[pos][j];
		for(int j=cutpoint;j<genome_size;j++) g[j]=genome[gpos][j];
		double f=fitness(g);
        if(fabs(f)>1e+10) goto again;
		if(fabs(f)<fabs(fitness_array[pos]))
		{
			printf("%lf ",f);
			fflush(stdout);
			for(int j=0;j<genome_size;j++) genome[pos][j]=g[j];
			fitness_array[pos]=f;
		}
		else
		{
			for(int j=0;j<cutpoint;j++) g[j]=genome[gpos][j];
			for(int j=cutpoint;j<genome_size;j++) g[j]=genome[pos][j];
			double f=fitness(g);
			if(fabs(f)<fabs(fitness_array[pos]))
			{
			printf("%lf ",f);
			fflush(stdout);
				for(int j=0;j<genome_size;j++) genome[pos][j]=g[j];
				fitness_array[pos]=f;
			}
		}
	}
}

void	Population::mutateItem(int pos)
{
	vector<int> g;
	g.resize(genome_size);
	for(int i=0;i<genome_size;i++) g[i]=genome[pos][i];
    printf("LOCAL[%d] = ",pos);
    fflush(stdout);
	for(int j=0;j<10;j++)
       for(int i=0;i<genome_size;i++)
       {
                {
                        int ik=0;
                        double f;
                        do
                        {
                                g[i]=rand() % MAX_RULE;
                                ik++;
                                if(ik==10) break;
                                f=fitness(g);
                        }while(f<=fitness_array[pos]);
                        if(ik!=10)
                        {
                                fitness_array[pos]=f;
				printf(" %lf ",f);
				
				fflush(stdout);
                                genome[pos][i]=g[i];
                        }
                        else g[i]=genome[pos][i];
                }
        }
	printf("\n");
}


void    Population::mutateItemAtClass(int pos,int classIndex)
{
    vector<int> g;
    g.resize(genome_size);
    for(int i=0;i<genome_size;i++) g[i]=genome[pos][i];
    printf("LOCAL PERCLASS[%d] = ",pos);
    fflush(stdout);
    ClassProgram *p=(ClassProgram *)program;
    int partSize = genome_size/(p->getClass()-1);
    int start  = (classIndex)*partSize;
    int end = (classIndex+1)*partSize;
    if(end>=genome_size) end--;
    for(int j=0;j<10;j++)
        for(int i=start;i<end;i++)
        {
            {
                int ik=0;
                double f;
                do
                {
                    g[i]=rand() % MAX_RULE;
                    ik++;
                    if(ik==10) break;
                    f=fitness(g);
                }while(f<=fitness_array[pos]);
                if(ik!=10)
                {
                    fitness_array[pos]=f;
                    printf(" %lf ",f);

                    fflush(stdout);
                    genome[pos][i]=g[i];
                }
                else g[i]=genome[pos][i];
            }
        }
    printf("\n");

}

void        Population::localSearch(int pos)
{
	vector<int> g;
	g.resize(genome_size);
	for(int i=0;i<genome_size;i++) g[i]=genome[pos][i];

    if(localMethod == "crossover")
    {
	    crossItem(pos);
    }
    else
    if(localMethod == "mutateWorst")
    {
        ClassProgram *p=(ClassProgram *)program;
        vector<double> val;
        val.resize(p->getClass());
        p->getErrorPerClass(g,val);
        int maxIndex=0;
        double maxValue=val[0];
        for(int i=0;i<val.size();i++)
        {
            if(val[i]>maxValue)
            {
                maxIndex = i;
                maxValue = val[i];
            }
        }
        printf("MUTATE %d Index with Value %lf \n",
               maxIndex,maxValue);
        if(maxIndex==p->getClass()-1)
            mutateItem(pos);
        else
        mutateItemAtClass(pos,maxIndex);
    }
    else
    if(localMethod == "mutate")
    {
	    mutateItem(pos);
    }
    else
    if(localMethod == "siman")
    {
            double f = fitness_array[pos];
            g=simulatedAnnealing(g);
            fitness_array[pos]=fitness(g);
            for(int j=0;j<genome_size;j++) genome[pos][j]=g[j];

            printf("SIMAN[%d] %lf=>%lf\n",pos,f,fitness_array[pos]);
    }
    else
    if(localMethod=="de")
    {
        int randomA,randomB,randomC;
         do
         {
             randomA =  rand() % genome_count;
             randomB =  rand() % genome_count;
             randomC =  rand() % genome_count;
         }while(randomA == randomB || randomB == randomC || randomC == randomA);
         double CR= 0.9;
         double F = 0.8;
         int randomIndex = rand() % genome_size;
     for(int i=0;i<genome_size;i++)
     {
         if(i==randomIndex || rand()*1.0/RAND_MAX <=CR)
         {
             int old_value = genome[pos][i];
             F = -0.5 + 2.0 * rand()*1.0/RAND_MAX;
             genome[pos][i]=genome[randomA][i]+abs(F*(genome[randomB][i]-genome[randomC][i]));
             if(genome[pos][i]<0)
             {
              genome[pos][i]=old_value;
              continue;
             }

             for(int j=0;j<genome_size;j++) g[j]=genome[pos][j];
             double trial_fitness=fitness(g);
             if(fabs(trial_fitness)<fabs(fitness_array[pos]))
             {
                //printf("DE. NEW VALUE[%d] = %lf=>%lf\n",pos,fitness_array[pos],trial_fitness);
                fitness_array[pos]=trial_fitness;
             }
             else	genome[pos][i]=old_value;
         }
    }
    }
    else
        if(localMethod=="gd")
    {
        integerLocalSearch(g);
        double ff = fitness(g);
        if(fabs(ff)<fabs(fitness_array[pos]))
        {
            printf("GD. NEW VALUE[%d] = %lf=>%lf\n",pos,fitness_array[pos],ff);
            fitness_array[pos]=ff;
            for(int j=0;j<g.size();j++) genome[pos][j]=g[j];
        }
    }
    else
        if(localMethod == "adam")
    {
        g=integerAdam(g);
        double ff = fitness(g);
        if(fabs(ff)<fabs(fitness_array[pos]))
        {
            printf("ADAM. NEW VALUE[%d] = %lf=>%lf\n",pos,fitness_array[pos],ff);
            fitness_array[pos]=ff;
            for(int j=0;j<g.size();j++) genome[pos][j]=g[j];
        }
    }
	
}

/* Evaluate and return the best fitness for all chromosomes in the population */
double	Population::evaluateBestFitness() 
{
	vector<int> g;g.resize(genome_size);
	for(int i=0;i<genome_size;i++) g[i]=genome[0][i];	
	return fitness(g);
}

/* Destructor */
Population::~Population()
{
	for(int i=0;i<genome_count;i++)
	{
		delete[] children[i];
		delete[] genome[i];
	}
	delete[] genome;
	delete[] children;
	delete[] fitness_array;
}
