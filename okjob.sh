#!/bin/bash

#SBATCH --partition=gpu
#SBATCH --qos=debug
#SBATCH --account=junogpu
#SBATCH --job-name=okjob
#SBATCH --ntasks=1
#SBATCH --output=/hpcfs/juno/junogpu/blyth/okjob/%j.out
#SBATCH --error=/hpcfs/juno/junogpu/blyth/okjob/%j.err
#SBATCH --mem-per-cpu=20480
#SBATCH --gres=gpu:v100:1

okjob-notes(){ cat << EON

Before submitting this job with "sb" check the script 
on gateway lxslc7 with::

   /hpcfs/juno/junogpu/blyth/j/okjob.sh
   
Around 15 tests should fail from lack of GPU    

EON
}

okjob-setup()
{
    export OPTICKS_CUDA_PREFIX=/usr/local/cuda-11.7

    ## below duplicates .opticks_usage_config as its better for jobs to be self contained
    source /cvmfs/juno.ihep.ac.cn/centos7_amd64_gcc1120/Pre-Release/J23.1.x/ExternalLibs/Boost/1.78.0/bashrc
    source /cvmfs/juno.ihep.ac.cn/centos7_amd64_gcc1120/Pre-Release/J23.1.x/ExternalLibs/Xercesc/3.2.3/bashrc
    source /cvmfs/juno.ihep.ac.cn/centos7_amd64_gcc1120/Pre-Release/J23.1.x/ExternalLibs/CLHEP/2.4.1.0/bashrc
    source /cvmfs/juno.ihep.ac.cn/centos7_amd64_gcc1120/Pre-Release/J23.1.x/ExternalLibs/Geant4/10.04.p02.juno/bashrc

    vers=Opticks-0.0.1_alpha/x86_64-CentOS7-gcc1120-geant4_10_04_p02-dbg

#   source /hpcfs/juno/junogpu/blyth/local/$vers/bashrc                   ## copied in tarball created on workstation 
#   source /hpcfs/juno/junogpu/blyth/local/opticks_release/$vers/bashrc   ## expanded locally built tarball
    source /hpcfs/juno/junogpu/blyth/local/opticks/bashrc                 ## local build 


    export HOME=/hpcfs/juno/junogpu/$USER
    export TMP=$HOME/tmp   ## override default /tmp/$USER/opticks as /tmp is blackhole (not same filesystem on GPU cluster and gateway)  
    mkdir -p $TMP          ## whether override or not, need to create 
    mkdir -p $HOME/okjob
}

okjob-setup-notes(){ cat << EON

* HMM: should OPTICKS_CUDA_PREFIX be captured by the build and hardcoded into the bashrc ? 

EON
}


okjob-head(){ 
   hostname 
   nvidia-smi   
   local vars="0 HOSTNAME USER BASH_SOURCE HOME OPTICKS_PREFIX CUDA_VISIBLE_DEVICES"
   for var in $vars ; do printf "%25s : %s \n" "$var" "${!var}" ; done 

   unset CUDA_VISIBLE_DEVICES
   nvidia-smi   

   date 
}
okjob-body()
{
   local msg="=== $FUNCNAME:"
   echo $msg TMP $TMP

   cd $OPTICKS_PREFIX/tests

   which ctest 
   ctest -N 
   ctest --output-on-failure

}
okjob-tail(){
   local rc=$?    # capture the return code of prior command
   echo $FUNCNAME : rc $rc              
   date  
   
   ## /tmp on gpu cluster is a black hole 
   ## any tests still writing need to be changed to use $TMP
   ls -alst /tmp | grep blyth
   [ -d /tmp/blyth ] && find /tmp/blyth -type f 

}

okjob-setup
okjob-head
okjob-body
okjob-tail
