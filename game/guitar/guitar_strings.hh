#pragma once

using Frequency = float;

enum class StringName {
	E_Low, A, D, G, B, E_High
};

class GuitarStrings {
public:
	Frequency getFrequency(StringName string, int fret = 0) const;
	Frequency getBaseFrequency(StringName string) const;
};
