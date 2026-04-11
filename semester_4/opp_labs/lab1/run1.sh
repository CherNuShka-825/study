#!/bin/bash
#PBS -q myq
#PBS -l walltime=00:05:00
#PBS -l select=2:ncpus=8:mpiprocs=8
#PBS -m n

cd $PBS_O_WORKDIR

MPI_NP=$(wc -l $PBS_NODEFILE | awk '{print $1}')

echo "Number of MPI processes: $MPI_NP"
echo "Nodefile:"
cat $PBS_NODEFILE
echo

mpirun -machinefile $PBS_NODEFILE -np $MPI_NP ./type-1