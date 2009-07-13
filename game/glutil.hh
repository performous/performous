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
}

