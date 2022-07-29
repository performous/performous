#pragma once

class BiQuad {
public:
	float operator()(float x) {
		y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
		x2 = x1;
		x1 = x;
		y2 = y1;
		y1 = y;

		return y;
	}

private:
	float x1 = 0.f;
	float x2 = 0.f;
	float y = 0.f;
	float y1 = 0.f;
	float y2 = 0.f;
	// Coefficients: https://arachnoid.com/BiQuadDesigner/index.html
	float a1 = -1.92089585f;
	float a2 = 0.923907730f;
	float b0 = 1.002346250f;
	float b1 = -1.92071210f;
	float b2 = 0.921745230f;
};

