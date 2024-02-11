#pragma once

#include "bitmap.hh"

namespace {
	float calculate(unsigned value, unsigned size) {
		return static_cast<float>(value) / static_cast<float>(size);
	}
	float calculate(unsigned value, float size) {
		return static_cast<float>(value) / size;
	}
}

/// texture coordinates
struct TexCoords {
	float x1 = 0.0f; ///< left
	float y1 = 0.0f; ///< top
	float x2 = 1.0f; ///< right
	float y2 = 1.0f; ///< bottom

	TexCoords() = default;
	TexCoords(float x1_, float y1_, float x2_, float y2_) :
		x1(x1_), y1(y1_), x2(x2_), y2(y2_) {
	}
	TexCoords(Clip const& clip, unsigned width, unsigned height) {
		x1 = calculate(clip.left, width);
		y1 = calculate(clip.top, height);
		x2 = 1.f - calculate(clip.right, width);
		y2 = 1.f - calculate(clip.bottom, height);
	}
	TexCoords(Clip const& clip, float width, float height) {
		x1 = calculate(clip.left, width);
		y1 = calculate(clip.top, height);
		x2 = 1.f - calculate(clip.right, width);
		y2 = 1.f - calculate(clip.bottom, height);
	}
	TexCoords(unsigned left, unsigned top, unsigned right, unsigned bottom, float width, float height) {
		x1 = calculate(left, width);
		y1 = calculate(top, height);
		x2 = 1.f - calculate(right, width);
		y2 = 1.f - calculate(bottom, height);
	}
	bool outOfBounds() const {
		return isOutOfRange(x1) || isOutOfRange(y1) || isOutOfRange(x2) || isOutOfRange(y2);
	}
private:
	static bool isOutOfRange(float x) {
		return x < 0.0f || x > 1.0f; 
	}
};
