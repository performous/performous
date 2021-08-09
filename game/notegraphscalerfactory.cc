#include "notegraphscalerfactory.hh"

#include "dynamicnotegraphscaler.hh"
#include "fixednotegraphscaler.hh"
#include "notescaleanalyser.hh"

#include "configuration.hh"

NoteGraphScalerPtr NoteGraphScalerFactory::create(VocalTrack const& vocal) const {
    const auto scalingMode = config["game/notegraphscalingmode"].i();

    if(scalingMode == 2) {
        const auto octaves = vocal.noteMax - vocal.noteMin;
        
        if(octaves <= 18)
            return std::make_shared<FixedNoteGraphScaler>();

        return std::make_shared<DynamicNoteGraphScaler>();
    }
    
    if(scalingMode == 1)
        return std::make_shared<FixedNoteGraphScaler>();
        
    return std::make_shared<DynamicNoteGraphScaler>();
}
