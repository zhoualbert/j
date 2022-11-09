/**
LayrTest.cc
=============

TODO: integrate with JPMT.h so can make realistic plots 

**/

#include "sdomain.h"

#include "LayrTest.h"
#include "JPMT.h"


template<typename T>
void test_scan(const JPMT& jp, int wavelength, int pmtcat)
{
    const char* pmtcat_label = jp.get_pmtcat(pmtcat) ; 
    T wavelength_nm = wavelength ; 
    std::cout << " pmtcat " << pmtcat << " pmtcat_label " << pmtcat_label << std::endl ;  

    StackSpec<T> spec( pmtcat == -1 ? StackSpec<T>::EGet() : jp.get(pmtcat, wavelength_nm ) ); 
    std::cout << spec << std::endl ; 

    int ni = 900 ; 

    LayrTest<T,4> t0(ni, wavelength_nm, pmtcat_label) ; 
    t0.scan_cpu(spec) ;

#ifdef WITH_THRUST
    LayrTest<T,4> t1(ni, wavelength_nm, pmtcat_label ) ; 
    t1.scan_gpu(spec) ;
#endif
}

int main(int argc, char** argv)
{
    JPMT jp ; 
    std::cout << jp.desc() << std::endl ; 

    int wl = 400 ; 
    //int pmtcat = JPMT::HAMA ;
    //int pmtcat = JPMT::NNVT ;
    int pmtcat = JPMT::NNVTQ ;

    test_scan<double>(jp, wl, pmtcat ); 
    test_scan<float>( jp, wl, pmtcat ); 

    return 0 ; 
}
