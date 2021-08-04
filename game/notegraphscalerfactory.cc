#include "notegraphscalerfactory.hh"

#include "dynamicnotegraphscaler.hh"
#include "fixednotegraphscaler.hh"

#include "configuration.hh"

NoteGraphScalerPtr NoteGraphScalerFactory::create() const {
    const auto scalingMode = config["game/notegraphscalingmode"].i();

    if(scalingMode == 1)
        return std::make_shared<FixedNoteGraphScaler>();
        
    return std::make_shared<DynamicNoteGraphScaler>();
}
