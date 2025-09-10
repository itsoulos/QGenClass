## SCRIPT TO EXECUTE THE  GENCLASS

## DATAPATH: the location of datasets
DATAPATH=~/Desktop/ERGASIES/FeatureConstruction2/datasets/tenfolding/

## PROGRAM: The name of the executable
PROGRAM=./QGenClass

## DATAFILE: the name of the used dataset
DATAFILE=$1

### GENPRAMS: The params of genetic algorithm
## pop_count: Chromosome count.
## pop_size:  Size of each chromosome.
## pop_gens:  Maximum number of generations.
## pop_srate: Selection rate.
## pop_mrate: Mutation rate.
## pop_lrate: Local search rate.
## pop_lmethod: Local search method. Available methods: none, crossover, mutate, siman.
## pop_fitnessmethod: Fitness calculation method. Available methods: class, average, squared, mixed
## pop_classpercent:   The percentage of class mode in mixed mode fitness.
## pop_averagepercent: The percentage of average mode in mixed mode fitness.
## pop_squaredpercent: The percentage of squared mode in mixed mode fitness.
GENPARAMS="--pop_count=500 --pop_size=200 --pop_gens=200 --pop_srate=0.1 --pop_mrate=0.05 --pop_lrate=0.001 --pop_lmethod=crossover --pop_fitnessmethod=mixed --pop_classpercent=0.5 --pop_averagepercent=0.25 --pop_squaredpercent=0.25"
# TERMINATION: The termination method used. Available values: maxiters, doublebox, similarity
TERMINATION=maxiters

DATAPARAMS="--train_file=$DATAPATH/$DATAFILE.train --train_format=data --test_file=$DATAPATH/$DATAFILE.test --test_format=data"
## Execute the program
$PROGRAM $GENPARAMS $DATAPARAMS
