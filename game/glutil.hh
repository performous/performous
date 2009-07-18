#pragma once

#include <GL/glew.h>

namespace glutil {
	struct PushMatrix {
		PushMatrix() { glPushMatrix(); }
		~PushMatrix() { glPopMatrix(); }
	};

	struct Begin {
		Begin(GLint mode) { glBegin(mode); }
		~Begin() { glEnd(); }
	};
	
	struct Color {
		float r, g, b, a;
		Color(float r_, float g_, float b_, float a_ = 1.0): r(r_), g(g_), b(b_), a(a_) {}
		operator float*() { return reinterpret_cast<float*>(this); }
		operator float const*() const { return reinterpret_cast<float const*>(this); }
	};
}

