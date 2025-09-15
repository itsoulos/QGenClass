## SCRIPT TO EXECUTE THE  GENCLASS

## DATAPATH: the location of datasets
DATAPATH=~/Desktop/ERGASIES/FeatureConstruction2/datasets/tenfolding/

## PROGRAM: The name of the executable
PROGRAM=./QGenClass

## DATAFILE: the name of the used dataset
DATAFILE=$1

SEED=1
if [ -z $2 ] 
then
	SEED="1"
else
	SEED=$2
fi

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
GENPARAMS="--pop_count=500 --pop_size=200 --pop_gens=2000 --pop_srate=0.1 --pop_mrate=0.05 --pop_lrate=0.001 --pop_lmethod=none --pop_fitnessmethod=class --pop_classpercent=0.5 --pop_averagepercent=0.25 --pop_squaredpercent=0.25 --random_seed=$SEED"
# TERMINATION: The termination method used. Available values: maxiters, doublebox, similarity
TERMINATION=maxiters

### DATAPARAMS: The parameters used for datasets handling.
## train_file:   The path to the train file
## train_format: The used format for the train file. Available values: data, csv, arff.
## test_file:    The path to the test file.
## test_format:  The used format for the test file. Available values: data, csv, arff.
## enable_smote: This is a flag that enables or disables the Smote method in the train set. Values: yes/no
## smote_k     : An integer value used in the k-nn procedure of the smote method.
DATAPARAMS="--train_file=$DATAPATH/$DATAFILE.train --train_format=data --test_file=$DATAPATH/$DATAFILE.test --test_format=data --enable_smote=no --smote_k=5"
## Execute the program
$PROGRAM $GENPARAMS $DATAPARAMS
