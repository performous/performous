#pragma once

#include <string>
#include <map>
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

	/** Get uniform location. Uses caching internally. */
	GLint operator[](const std::string& uniform);

	/** Allow setting uniforms in a chain. Shader needs to be in use.*/

	Shader& setUniform(const std::string& uniform, int value) {
		glUniform1i((*this)[uniform], value);
		return *this;
	}
	Shader& setUniform(const std::string& uniform, float value) {
		glUniform1f((*this)[uniform], value);
		return *this;
	}

	// Some operators
	operator bool() const { return program != 0; }
	bool operator==(const Shader& rhs) const { return program == rhs.program; }
	bool operator!=(const Shader& rhs) const { return program != rhs.program; }

	/** Returns pointer to the currently used shader. */
	static Shader* current() {
		GLint i;
		glGetIntegerv(GL_CURRENT_PROGRAM, &i);
		return shaders[i];
	}

	GLuint program, vert_shader, frag_shader; ///< shader object ids
	int gl_response;

  private:
	typedef std::map<std::string, GLint> UniformMap;
	UniformMap uniforms; ///< Cached uniform locations, use operator[] to access

	typedef std::map<GLint, Shader*> ShaderMap;
	static ShaderMap shaders; ///< Shader objects for reverse look-up by id
};


/** Temporarily switch shader in a RAII manner. */
struct UseShader {
	UseShader(Shader& new_shader) {
		glGetIntegerv(GL_CURRENT_PROGRAM, &m_old);
		new_shader.bind();
	}
	~UseShader() { glUseProgram(m_old); }

  private:
	GLint m_old;
};


/** Load shaders. */
void loadShaders();
