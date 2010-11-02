#pragma once

#include <string>
#include <GL/glew.h>
#include <boost/noncopyable.hpp>


struct Shader: public boost::noncopyable {
	Shader();
	Shader(const std::string& vert_path, const std::string& frag_path, bool use = false);
	~Shader();

	/** Loads the shader from files. */
	void loadFromFile(const std::string& vert_path, const std::string& frag_path, bool use = false);
	/** Loads the shader from memory. */
	void loadFromMemory(const char* vert_source, const char* frag_source, bool use = false);

	/** Binds the shader into use. */
	void bind();

	GLuint program, vert_shader, frag_shader; ///< shader object ids
	GLint tex, texRect, texMode; ///< uniform locations
	int gl_response;
};


/** Temporarily switch shader in a RAII manner. */
struct UseShader {
	UseShader(Shader& new_shader, Shader& old_shader): m_old(old_shader) {
		new_shader.bind();
	}
	~UseShader() { m_old.bind(); }
  private:
	Shader& m_old;
};


/** Load shaders. */
void loadShaders();
