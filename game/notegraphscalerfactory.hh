#pragma once

#include "inotegraphscaler.hh"

class NoteGraphScalerFactory
{
public:
	NoteGraphScalerPtr create() const;
};
