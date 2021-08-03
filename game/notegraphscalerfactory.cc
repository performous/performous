#include "notegraphscalerfactory.hh"

#include "dynamicnotegraphscaler.hh"
#include "fixnotegraphscaler.hh"

#include "configuration.hh"

NoteGraphScalerPtr NoteGraphScalerFactory::create() const {
	const auto scalingMode = config["game/notegraphscalingmode"].i();

	if(scalingMode == 1)
		return std::make_shared<FixNoteGraphScaler>();
		
	return std::make_shared<DynamicNoteGraphScaler>();
}
