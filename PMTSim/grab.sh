#!/bin/bash -l 

cd $HOME/j/PMTSim

jcopy \
    HamamatsuR12860PMTManager \
    Hamamatsu_R12860_PMTSolid \
    ellipse_intersect_circle 


usage(){ cat << EOU

junoPMTOpticalModel


EOU
}

