#!/bin/bash -l
#Author Name:Yuxiang Hu
#Creation date:2024-01-30
#Description: 

#SBATCH --partition=gpu
#SBATCH --qos=debug
#SBATCH --account=junogpu
#SBATCH --job-name=script
#SBATCH --ntasks=1
#SBATCH --output=/hpcfs/juno/junogpu/%u/script/%j.out
#SBATCH --error=/hpcfs/juno/junogpu/%u/script/%j.err
#SBATCH --mem-per-cpu=20480
#SBATCH --gres=gpu:v100:1

source /cvmfs/juno.ihep.ac.cn/centos7_amd64_gcc1120_opticks/Pre-Release/J23.1.0-rc6/bashrc.sh # JUNOTOP set here
source $JUNOTOP/setup.sh

source $JUNOTOP/opticks/opticks.bash
source $JUNOTOP/opticks/om.bash

source ~/j/j.bash
source ~/j/jx.bash
source ~/j/jok.bash

home=/hpcfs/juno/junogpu/$USER
[ -d "$home" ] && export HOME=$home   
export TMP=$HOME/tmp   ## override default /tmp/$USER/opticks as /tmp is blackhole (not same filesystem on GPU cluster and gateway)  
mkdir -p $TMP          ## whether override or not, need to create 

export RNGDir="/cvmfs/opticks.ihep.ac.cn/.opticks/rngcache/RNG"

export LUT=0
export ENE=10  #MeV
export OIM=3   # Opticks Integration Mode
export GUN=4
export CATE="LUT_${LUT}_ENE_${ENE}_OIM_${OIM}_GUN_${GUN}"
jok-tds
