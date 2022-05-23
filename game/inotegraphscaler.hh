#pragma once

#include "notes.hh"

#include <memory>

class Song;
class Database;

struct NoteGraphDimension {
    float min1;
    float min2;
    float max1;
    float max2;
};

struct INoteGraphScaler {
    virtual ~INoteGraphScaler() = default;

    virtual void initialize(VocalTrack const&) = 0;
    virtual NoteGraphDimension calculate(VocalTrack const&, Notes::const_iterator const&, double time) const = 0;
};

using NoteGraphScalerPtr = std::shared_ptr<INoteGraphScaler>;
