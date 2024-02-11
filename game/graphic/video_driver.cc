#include "video_driver.hh"

#include "chrono.hh"
#include "config.hh"
#include "controllers.hh"
#include "fs.hh"
#include "glmath.hh"
#include "graphic/bitmap.hh"
#include "platform.hh"
#include "screen.hh"
#include "game.hh"
#include "util.hh"
#include "fs.hh"
#include <SDL.h>
#include <SDL_hints.h>
#include <SDL_rect.h>
#include <SDL_video.h>
#include <cstdint>

namespace Global {
	glmath::mat4 projection = glmath::mat4(1.0f);
	glmath::mat4 modelview = glmath::mat4(1.0f);
	glmath::mat4 color = glmath::mat4(1.0f);
}

float getSeparation() {
	return config["graphic/stereo3d"].b() ? 0.001f * config["graphic/stereo3dseparation"].f() : 0.0f;
}

glmath::mat4 farTransform() {
	float z = Constant::far - 0.1f;  // Very near the far plane but just a bit closer to avoid accidental clipping
	float s = z / Constant::z0;  // Scale the image so that it looks the same size
	s *= 1.0f + 2.0f * getSeparation(); // A bit more for stereo3d (avoid black borders)
	using namespace glmath;
	return translate(vec3(0.0f, 0.0f, -z + Constant::z0)) * scale(s); // Very near the farplane
}
