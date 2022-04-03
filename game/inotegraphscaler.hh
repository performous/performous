#pragma once

#include "notes.hh"

#include <memory>

class Song;
class Database;

struct NoteGraphDimension {
    int min1;
    int min2;    
    int max1;
    int max2;
};

struct INoteGraphScaler {
    virtual ~INoteGraphScaler() = default;
    
    virtual void initialize(VocalTrack const&) = 0;
    virtual NoteGraphDimension calculate(VocalTrack const&, Notes::const_iterator const&, double time) const = 0;
};

using NoteGraphScalerPtr = std::shared_ptr<INoteGraphScaler>;

