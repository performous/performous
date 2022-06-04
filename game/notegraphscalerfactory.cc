#include "notegraphscalerfactory.hh"

#include "dynamicnotegraphscaler.hh"
#include "fixednotegraphscaler.hh"

#include "configuration.hh"

NoteGraphScalerPtr NoteGraphScalerFactory::create(VocalTrack const& vocal) const {
    const auto &scalingMode = config["game/notegraphscalingmode"].getEnumName();

    if(scalingMode == "Dynamic")
        return std::make_shared<DynamicNoteGraphScaler>();
    if(scalingMode == "Fixed")
        return std::make_shared<FixedNoteGraphScaler>();
        
    const auto octaves = vocal.noteMax - vocal.noteMin;

    if(scalingMode == "Auto (1 octave)" && octaves <= 12)
        return std::make_shared<FixedNoteGraphScaler>();
    if(scalingMode == "Auto (1.5 octaves)" && octaves <= 18)
        return std::make_shared<FixedNoteGraphScaler>();
    if(scalingMode == "Auto (2 octaves)" && octaves <= 24)
        return std::make_shared<FixedNoteGraphScaler>();
    if(scalingMode == "Auto (2.5 octaves)" && octaves <= 30)
        return std::make_shared<FixedNoteGraphScaler>();
    
     return std::make_shared<DynamicNoteGraphScaler>();
}
