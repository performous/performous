#pragma once

#include <string>
#include <GL/glew.h>

struct Shader {
	Shader();
	Shader(const std::string& vert_path, const std::string& frag_path, bool use = true);
	~Shader();

	/** Loads the shader from files. */
	void loadFromFile(const std::string& vert_path, const std::string& frag_path, bool use = true);
	/** Loads the shader from memory. */
	void loadFromMemory(const char* vert_source, const char* frag_source, bool use = true);

	/** Binds the shader into use. */
	void bind() { glUseProgram(program); }

	GLuint program, vert_shader, frag_shader;
	int gl_response;
};

