#pragma once

#include "glmath.hh"

float getSeparation();

/// Performs a GL transform for displaying background image at far distance
glmath::mat4 farTransform();

namespace Global {
	extern glmath::mat4 projection;
	extern glmath::mat4 modelview;
	extern glmath::mat4 color;
}

namespace Constant {
	const float targetWidth = 1366.0f; // One of the most common desktop resolutions in use today.
	// stump: under MSVC, near and far are #defined to nothing for compatibility with ancient code, hence the underscores.
	const float farDistance = 110.0f; // How far away can things be seen
	const float nearDistance = 0.1f; // This determines the near clipping distance (must be > 0)
	const float z0 = 1.5f; // This determines FOV: the value is your distance from the monitor (the unit being the width of the Performous window)
}


