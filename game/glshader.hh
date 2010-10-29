#pragma once

#include <string>
#include <GL/glew.h>

struct Shader {
	Shader(const char* vertex_shader, const char* fragment_shader);
	Shader() {};
	~Shader();

	GLuint program, vert_shader, frag_shader;
	int gl_response;
};

