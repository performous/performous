#pragma once

#include "inotegraphscaler.hh"
#include "configuration.hh"

class NoteGraphScalerFactory
{
public:
    NoteGraphScalerFactory(Config&);
    
    NoteGraphScalerPtr create(VocalTrack const&) const;
    
private:
    Config& _configuration;
};
