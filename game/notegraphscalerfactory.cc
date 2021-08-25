#include "notegraphscalerfactory.hh"

#include "dynamicnotegraphscaler.hh"
#include "fixednotegraphscaler.hh"

NoteGraphScalerFactory::NoteGraphScalerFactory(Config& configuration)
: _configuration(configuration)
{
}

NoteGraphScalerPtr NoteGraphScalerFactory::create(VocalTrack const& vocal) const {
    const auto scalingMode = _configuration["game/notegraphscalingmode"].ui();

    if(scalingMode == 0)
        return std::make_shared<DynamicNoteGraphScaler>();
    if(scalingMode == 1)
        return std::make_shared<FixedNoteGraphScaler>();
        
    const auto octaves = vocal.noteMax - vocal.noteMin;

    if(scalingMode == 2 && octaves <= 12)
        return std::make_shared<FixedNoteGraphScaler>();
    if(scalingMode == 3 && octaves <= 18)
        return std::make_shared<FixedNoteGraphScaler>();
    if(scalingMode == 4 && octaves <= 24)
        return std::make_shared<FixedNoteGraphScaler>();
    if(scalingMode == 5 && octaves <= 30)
        return std::make_shared<FixedNoteGraphScaler>();
    
     return std::make_shared<DynamicNoteGraphScaler>();
}
