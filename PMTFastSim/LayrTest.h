#pragma once
/**
LayrTest.h
===========

HMM: how to arrange to allow most of this to be reused 
for both CPU and GPU testing ?

1. Pull out a simpler struct LayrTestData that just holds pointers
   to the arrays which can be passed to device holding 
   the device pointers. 


**/

#include "NP.hh"
#include "SU.hh"
#include "Layr.h"

template<typename T, int N>
struct LayrTestData
{
    int      ni ;   // number of angles 
    T        wl ; 
    T*       theta ; 
    ART<T>*  arts ; 
    Layr<T>* comps ;  
    Layr<T>* lls ; 
};


template<typename T, int N>
struct LayrTest
{
    LayrTestData<T,N> h ; // host
    LayrTestData<T,N> d ; // device
    LayrTestData<T,N>* d_ptr ; 

    LayrTest(int ni=90, T wl=0);

    void upload(); 
    void download(); 

    void scan(); 
    void scan(const StackSpec<T>& ss, bool reverse=false); 

    void save(const char* dir) const ; 
    std::string desc() const ; 
    std::string brief() const ; 
};

template<typename T, int N>
inline LayrTest<T,N>::LayrTest(int ni, T wl)
    :
    d_ptr(nullptr)
{
    h.ni = ni ; 
    h.wl = wl > 0 ? wl : U::GetE<double>("WL", 500.) ; 
    h.theta = new T[ni] ; 
    h.arts  = new ART<T>[ni] ; 
    h.comps = new Layr<T>[ni] ; 
    h.lls   = new Layr<T>[N*ni] ; 

    assert( sizeof(T) == 4 || sizeof(T) == 8 ); 
    for(int i=0 ; i < ni ; i++ ) 
    {
        //h.theta[i] = T(i)/T(ni-1)*M_PI/T(2) ; // ni angles from 0 to pi/2 radians
        h.theta[i] = T(i)/T(ni)*M_PI/T(2) ;     // ni angles from 0 to (89/90)*pi/2 radians : NOT QUITE ALL THE WAY
    }
}

template<typename T, int N>
inline void LayrTest<T,N>::upload()
{
    // prepare device side arrays
    int ni = h.ni ; 
    d.ni = ni ; 
    d.wl = h.wl ;
    d.theta = (T*)SU::device_alloc_sizeof(ni, sizeof(T)) ;     
    d.arts  = ( ART<T>*)SU::device_alloc_sizeof(ni, sizeof(ART<T>) ); 
    d.comps = (Layr<T>*)SU::device_alloc_sizeof(ni, sizeof(Layr<T>) ); 
    d.lls   = (Layr<T>*)SU::device_alloc_sizeof(ni, sizeof(Layr<T>)*N ); 

    SU::copy_host_to_device_sizeof( (char*)d.theta, (char*)h.theta, ni, sizeof(T) ); 

    d_ptr = (LayrTestData<T,N>*)SU::upload_array_sizeof(  (char*)&d, 1, sizeof(LayrTestData<T,N>) );  
}

template<typename T, int N>
inline void LayrTest<T,N>::download()
{
    int ni = d.ni ; 
    assert( d_ptr != nullptr ); // must upload before download
    SU::copy_device_to_host_sizeof( (char*)h.arts , (char*)d.arts , ni, sizeof(ART<T>) ); 
    SU::copy_device_to_host_sizeof( (char*)h.comps, (char*)d.comps, ni, sizeof(Layr<T>) ); 
    SU::copy_device_to_host_sizeof( (char*)h.lls  , (char*)d.lls  , ni, sizeof(Layr<T>)*N ); 
}


/**
LayrTest::scan
---------------

Initially it feels like a cheap trick (avoiding need for rerunable object) 
to instanciate the Stack object within the scan loop. 
But actually thats the real situation as the refractive 
indices depend on wavelength : so the matrix stack needs to be 
recomputed for every wl and angle.
Having a fixed wavelength to change angle with is purely artificial
whilst doing angle scanning. 

Get very large mismatch between float and double in lls and comp for th pi/2 (90 degrees). 
TODO: look into details to see if this glancing edge case needs some special treatment. 

**/

template<typename T, int N>
inline void LayrTest<T,N>::scan()
{
    StackSpec<T> spec(StackSpec<T>::Default()); 
    scan(spec); 
}
 
template<typename T, int N>
inline void LayrTest<T,N>::scan(const StackSpec<T>& spec, bool reverse)
{
    Stack<T,N> stack(h.wl, spec) ; 
    for(int i=0 ; i < h.ni ; i++ )
    {
        int j = reverse ? h.ni - 1 - i : i ;   // reverse is just a debugging trick 
        stack.computeART(h.theta[j]); 

        h.arts[j] = stack.art; 
        h.comps[j] = stack.comp ; 
        for(int l=0 ; l < N ; l++) h.lls[N*j+l] = stack.ll[l]; 

        //std::cout << stack << std::endl ; 
        //std::cout << "j:" << j << std::endl << stack.art << std::endl ; 
    }
}

template<typename T, int N>
inline std::string LayrTest<T,N>::desc() const 
{
    std::stringstream ss ; 
    for(int i=0 ; i < h.ni ; i++)
    {
        const ART<T>& art = h.arts[i] ; 
        ss << " i:" << i 
           << std::endl  
           << art 
           << std::endl 
           ; 
    }
    std::string s = ss.str();
    return s ; 
}


template<typename T, int N>
inline std::string LayrTest<T,N>::brief() const 
{
    std::stringstream ss ; 
    ss
        << "LayrTest<"  << ( sizeof(T) == 8 ? "double" : "float" ) << "," << N << ">"   
#ifdef WITH_THRUST
        << " WITH_THRUST "
#else
        << " not-WITH_THRUST "
#endif
#if defined(__CUDACC__) || defined(__CUDABE__)
        << " (GPU) "
#else
        << " (CPU) "
#endif
        << " ni " << h.ni         
        << " wl " << h.wl         
        << " theta[0] " << h.theta[0]
        << " theta[ni-1] " << h.theta[h.ni-1]
        ; 
    std::string s = ss.str(); 
    return s ; 
}

template<typename T, int N>
inline void LayrTest<T,N>::save(const char* dir) const 
{
    assert( sizeof(ART<T>)/sizeof(T) == 12 ); 
    assert( sizeof(Layr<T>)/sizeof(T) == 4*4*2 ); 

    NP::Write(dir,"comps.npy",(T*)h.comps, h.ni, 4, 4, 2 ) ;    // 1 composite layer (4,4,2) at each angle
    NP::Write(dir,"lls.npy",  (T*)h.lls  , h.ni, 4, 4, 4, 2 ) ; // 4 layers (4,4,2) at each angle 

    // use manual way for _arts so can set metadata
    NP* _arts = NP::Make<T>( h.ni, 3, 4 ); // one ART result (3,4) at each angle 
    _arts->read2( (T*)h.arts ); 
    _arts->set_meta<std::string>("brief", brief()); 
    _arts->set_meta<T>("wl", h.wl); 
    _arts->save(dir, "arts.npy" ); 
}



