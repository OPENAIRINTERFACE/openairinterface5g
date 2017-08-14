#!/bin/bash
#PBS -q batch
#PBS -N ul_dl_w_interfer_sim
#PBS -o log.out
#PBS -j oe
#PBS -m abe
if [! -d "/homes/sorby/Devel/simDataOutput/"];then
    mkdir /homes/sorby/Devel/simDataOutput/
fi
OPENAIR1_DIR=/homes/sorby/Devel/openair1/
cd $OPENAIR1_DIR
cd SIMULATION/LTE_PHY
make physim_secsys MeNBMUE=1 PBS_SIM=1
#./physim_secsys
