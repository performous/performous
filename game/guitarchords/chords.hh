#pragma once

#include "game/guitarchords/chord.hh"
#include "game/guitarchords/notes.hh"

const Chord E("E", {0, 2, 2, 1, 0, 0});
const Chord A("A", {FretNone, 0, 2, 2, 2, 0});
const Chord G("G", {3, 2, 0, 0, 0, 3});
const Chord D("D", {FretNone, FretNone, 0, 2, 3, 2});
const Chord C("C", {FretNone, 3, 2, 0, 1, 0});
const Chord F("F", {FretNone, FretNone, 3, 2, 1, 1});
const Chord B("B", {FretNone, FretNone, 4, 4, 4, 2});
const Chord Em("Em", {0, 2, 2, 0, 0, 0});
const Chord Am("Am", {FretNone, 0, 2, 2, 1, 0});
const Chord Dm("Dm", {FretNone, FretNone, 0, 2, 3, 1});
const Chord Cm("Cm", {FretNone, 3, 1, 0, 1, FretNone});
const Chord Fm("Fm", {FretNone, FretNone, 3, 1, 1, 1});

std::vector<Chord> const chords{E, A, G, D, C, F, B, Em, Am, Dm, Cm, Fm};
