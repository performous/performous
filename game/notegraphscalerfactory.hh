#pragma once

#include "inotegraphscaler.hh"

class NoteGraphScalerFactory
{
public:
    NoteGraphScalerPtr create(VocalTrack const&) const;
};
