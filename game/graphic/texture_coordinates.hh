#pragma once

/// texture coordinates
struct TexCoords {
	float x1; ///< left
	float y1; ///< top
	float x2; ///< right
	float y2; ///< bottom

	TexCoords(float x1_ = 0.0f, float y1_ = 0.0f, float x2_ = 1.0f, float y2_ = 1.0f) :
		x1(x1_), y1(y1_), x2(x2_), y2(y2_) {}
	bool outOfBounds() const {
		return test(x1) || test(y1) || test(x2) || test(y2);
	}
private:
	static bool test(float x) {
		return x < 0.0f || x > 1.0f; 
	}
};
