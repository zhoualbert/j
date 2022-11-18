#pragma once
/**
LayrTest.h
===========

Note simple structure making use of arrays of structs 
to allow reuse of the same code for both CPU and GPU running. 

* nvcc compilation mainly needs struct layout, rather than the methods

**/

#if defined(__CUDACC__) || defined(__CUDABE__)
#else
    #include "NP.hh"
    #ifdef WITH_THRUST
        #include "SU.hh"
        #include <cuda_runtime.h>
    #endif
#endif

#include "Layr.h"

/**
LayrTestData : acts as primary host<->device communication mechanism
-----------------------------------------------------------------------

Retaining all the comps, lls for every test item  
is for debugging purposes only. 

NB using simple pointers to arrays enables this struct 
to be used on both device and host and copied between them

**/

template<typename T, int N>
struct LayrTestData    // LayrScanData  better name ?
{
    int      ni ;     // number of items : currently angles 
    T        wl ;     // hmm could vary wl, by making this an array 
    T*       mct ;    // minus_cos_theta from -1 to 1, > 0 is backwards stack
    ART_<T>* arts ; 
    Layr<T>* comps ;  
    Layr<T>* lls ; 
};


/**
LayrTest
----------

* h : host side struct with arrays populated by scan_cpu (OR download from scan_gpu)  
* d : host side preparation of device side struct containing device pointers
* d_ptr : device pointer to uploaded copy of above d struct, populated by scan_gpu 

**/

template<typename T, int N>
struct LayrTest
{
    LayrTestData<T,N> h ;      
    LayrTestData<T,N> d ;      
    LayrTestData<T,N>* d_ptr ; 

    bool             gpu ; 
    const char*      base ; 
    const char*      label ; 
    bool half ; 

#if defined(__CUDACC__) || defined(__CUDABE__)
#else
    LayrTest(int ni=90, T wl=0, const char* label=nullptr );

#ifdef WITH_THRUST
    void upload(); 
    void download(); 
    void scan_gpu(const StackSpec<T>& spec); 
#else
    // without thrust::complex cannot run on GPU 
#endif
    void scan_cpu(const StackSpec<T>& spec); 

    const char* get_name() const ; 
    void save() const ; 
    std::string desc() const ; 
    std::string brief() const ; 
    std::string title() const ; 
#endif
};

#if defined(__CUDACC__) || defined(__CUDABE__)
#else
template<typename T, int N>
inline LayrTest<T,N>::LayrTest(int ni, T wl, const char* label_ )
    :
    d_ptr(nullptr),
    gpu(false),    // flipped true/false by calling scan_gpu/scan_cpu
    base(U::GetEnv("LAYRTEST_BASE", "/tmp/LayrTest")),
    label(label_ ? strdup(label_) : nullptr),
    half(U::GetEnvInt("LAYRTEST_HALF",0) == 1)
{
    assert( sizeof(T) == 4 || sizeof(T) == 8 ); 
    h.ni = ni ; 
    h.wl = wl > 0. ? wl : U::GetE<double>("WL", 500.) ; 
    h.mct = new T[ni] ; 
    h.arts  = new ART_<T>[ni] ; 
    h.comps = new Layr<T>[ni] ; 
    h.lls   = new Layr<T>[N*ni] ; 

    T max_theta_pi = half ? T(1)/T(2) : T(1) ;  
    bool end_one = half ? false : true ;

    // HMM: when covering full 0->180 end_one does not
    // protect from glancing 90 degree incidence 
    //
    // get very large float/double mismatch in lls and comps 
    //
    // TODO: look into details to see why glancing edge case is so bad, 
    // maybe it needs some special treatment. 
 
    for(int i=0 ; i < ni ; i++ ) 
    {
        T frac =  T(i)/T(end_one ? ni-1 : ni) ;  
        T theta = frac*max_theta_pi*M_PI ; 
        h.mct[i] = -cos(theta) ;  
    }
}

#ifdef WITH_THRUST
template<typename T, int N>
inline void LayrTest<T,N>::upload()   // prepare device side arrays
{
    int ni = h.ni ; 
    d.ni = ni ; 
    d.wl = h.wl ;

    d.mct   =       (T*)SU::device_alloc_sizeof(ni, sizeof(T)) ;     
    d.arts  = (ART_<T>*)SU::device_alloc_sizeof(ni, sizeof(ART_<T>) ); 
    d.comps = (Layr<T>*)SU::device_alloc_sizeof(ni, sizeof(Layr<T>) ); 
    d.lls   = (Layr<T>*)SU::device_alloc_sizeof(ni, sizeof(Layr<T>)*N ); 

    printf("// upload h.mct[0] %10.4f h.mct[ni-1] %10.4f \n", h.mct[0], h.mct[ni-1] ); 
    SU::copy_host_to_device_sizeof( (char*)d.mct, (char*)h.mct, ni, sizeof(T) ); 

    d_ptr = (LayrTestData<T,N>*)SU::upload_array_sizeof((char*)&d, 1, sizeof(LayrTestData<T,N>) );  
}

template<typename T, int N>
inline void LayrTest<T,N>::download() // d->h : copy device side arrays down into host side 
{
    int ni = d.ni ; 
    assert( d_ptr != nullptr ); // must upload before download
    SU::copy_device_to_host_sizeof( (char*)h.arts , (char*)d.arts , ni, sizeof(ART_<T>) ); 
    SU::copy_device_to_host_sizeof( (char*)h.comps, (char*)d.comps, ni, sizeof(Layr<T>) ); 
    SU::copy_device_to_host_sizeof( (char*)h.lls  , (char*)d.lls  , ni, sizeof(Layr<T>)*N ); 
}

template <typename T, int N>
extern void LayrTest_launch( LayrTest<T,N>& t, const StackSpec<T>& spec ); 

template<typename T, int N>
inline void LayrTest<T,N>::scan_gpu(const StackSpec<T>& spec)
{
    gpu = true ; 

    upload();                           // prepare device side arrays
    LayrTest_launch<T,N>(*this, spec) ; // populate device side arrays

    cudaDeviceSynchronize();
    download();                         // copy d->h (overwriting any prior scan from scan_cpu OR scan_gpu)
    save();                             // persist the h arrays 
}
#endif


/**
LayrTest::scan_cpu
---------------------

Stack does everything in ctor because any change in wl or th 
demands almost everything is recomputed anyhow 

NOTE THE WL DEPENDENCY COMING IN TWICE : IN WL AND SPEC

**/
 
template<typename T, int N>
inline void LayrTest<T,N>::scan_cpu(const StackSpec<T>& spec)
{
    gpu = false ; 
    bool reverse = false ; 
    for(int i=0 ; i < h.ni ; i++ )
    {
        int j = reverse ? h.ni - 1 - i : i ;      // just debugging reorder
        Stack<T,N> stack(h.wl, h.mct[j], spec ) ; // ART_ calc done in ctor

        h.arts[j] = stack.art; 
        h.comps[j] = stack.comp ; 
        for(int l=0 ; l < N ; l++) h.lls[N*j+l] = stack.ll[l]; 

        //std::cout << stack << std::endl ; 
        //std::cout << "j:" << j << std::endl << stack.art << std::endl ; 
    }
    save(); 
}

template<typename T, int N>
inline std::string LayrTest<T,N>::desc() const 
{
    std::stringstream ss ; 
    for(int i=0 ; i < h.ni ; i++)
    {
        const ART_<T>& art = h.arts[i] ; 
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
    const char* name = get_name() ; 
    std::stringstream ss ; 
    ss
        << "LayrTest"
        << "<"  
        << ( sizeof(T) == 8 ? "double" : "float" ) 
        << "," 
        << N 
        << ">"   
#ifdef WITH_THRUST
        << " WITH_THRUST "
#else
        << " not-WITH_THRUST "
#endif
        << " name " << name
        << " ni " << h.ni         
        << " wl " << h.wl         
        << " mct[0] " << h.mct[0]
        << " mct[ni-1] " << h.mct[h.ni-1]
        ; 
    std::string s = ss.str(); 
    return s ; 
}

template<typename T, int N>
inline std::string LayrTest<T,N>::title() const 
{
    const char* name = get_name() ; 
    std::stringstream ss ; 
    ss
        << "j/Layr/LayrTest"
        << " " << name
        << " ni " << h.ni         
        << " wl " << h.wl         
        ; 
    std::string s = ss.str(); 
    return s ; 
}

template<typename T, int N>
inline const char* LayrTest<T,N>::get_name() const 
{
    std::stringstream ss ; 
    ss 
       << "scan__" 
       << ( label ? label : "" )
       << "__"
       << ( gpu ? "gpu" : "cpu" ) 
       << "_" 
#ifdef WITH_THRUST
       << "thr"
#else
       << "std"
#endif
       << "_" 
       << ( sizeof(T) == 8 ? "double" : "float" ) 
       ;
    std::string s = ss.str(); 
    return strdup(s.c_str()) ; 
}

template<typename T, int N>
inline void LayrTest<T,N>::save() const 
{
    std::string ti = title(); 
    std::string br = brief(); 
    std::cout << " title " << ti << std::endl ;  
    std::cout << " brief " << br << std::endl ;  

    assert( sizeof(ART_<T>)/sizeof(T) == 12 ); 
    assert( sizeof(Layr<T>)/sizeof(T) == 4*4*2 ); 

    const char* name = get_name() ; 
    NP::Write(base, name,"comps.npy",(T*)h.comps, h.ni,    4, 4, 2 ) ;
    NP::Write(base, name,"lls.npy",  (T*)h.lls  , h.ni, N, 4, 4, 2 ) ;

    // use manual way for _arts so can set metadata
    NP* _arts = NP::Make<T>( h.ni, 3, 4 );
    _arts->read2( (T*)h.arts ); 
    _arts->set_meta<std::string>("title", ti); 
    _arts->set_meta<std::string>("brief", br); 
    _arts->set_meta<std::string>("name", name); 
    _arts->set_meta<std::string>("label", label); 
    _arts->set_meta<T>("wl", h.wl); 
    _arts->save(base, name, "arts.npy" ); 
}
#endif
