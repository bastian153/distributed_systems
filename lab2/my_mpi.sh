#!/bin/bash
#
#$ -cwd
#$ -j y
#$ -S /bin/bash
#$ -M rcolumbu@uci.edu
#$ -pe openmpi 16
#$ -o result.out
#
# Use modules to setup the runtime environment
module load sge 
module load gcc/5.2.0
module load openmpi/1.6
#
# Execute the run
#
mpirun -np $NSLOTS ./C Timing_4.txt
