#pragma once

#include <vector>
class G4VSolid ; 
class G4LogicalVolume ; 

#include "PMTSIM_API_EXPORT.hh"

struct PMTSIM_API PMTSim
{
    static G4VSolid* GetSolid( const char* name="PMTSim_Z");  // for zcut solid include number in name eg "PMTSim_Z-400"
    static G4LogicalVolume* GetLV(const char* name); 
    static void Extract( std::vector<long>& vals, const char* s ); 
};


