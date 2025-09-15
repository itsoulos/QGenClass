#!/bin/sh
export LC_ALL=C
BASEPATH=~/Desktop/ERGASIES/QGenClass/
PROGRAM=$BASEPATH/runmodel.sh
DATAFILE=$1
OUTFILE=$1.out
INSTANCES=10
for i in $(seq 1 $INSTANCES)
do
$PROGRAM $DATAFILE $i >> $OUTFILE
echo "DATA: $1. Finish $i from $INSTANCES"
done

echo -n "CLASS ERROR: "
cat   $1.out  | grep PRECISION | awk '{print $2}' | awk '{ sum += $1 } END { if (NR > 0) print sum / NR }'

echo -n "PRECISION: "
cat   $1.out  | grep PRECISION | awk '{print $4}' | awk '{ sum += $1 } END { if (NR > 0) print sum / NR }'

echo -n "RECALL: "
cat   $1.out  | grep PRECISION | awk '{print $6}' | awk '{ sum += $1 } END { if (NR > 0) print sum / NR }'

echo -n "F1 SCORE: "
cat   $1.out  | grep PRECISION | awk '{print $8}' | awk '{ sum += $1 } END { if (NR > 0) print sum / NR }'
