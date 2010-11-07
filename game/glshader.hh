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
		glUniform1i((*this)[uniform], value); return *this;
	}
	Shader& setUniform(const std::string& uniform, float value) {
		glUniform1f((*this)[uniform], value); return *this;
	}
	Shader& setUniform(const std::string& uniform, int x, int y) {
		glUniform2i((*this)[uniform], x, y); return *this;
	}
	Shader& setUniform(const std::string& uniform, float x, float y) {
		glUniform2f((*this)[uniform], x, y); return *this;
	}
	Shader& setUniform(const std::string& uniform, int x, int y, int z) {
		glUniform3i((*this)[uniform], x, y, z); return *this;
	}
	Shader& setUniform(const std::string& uniform, float x, float y, float z) {
		glUniform3f((*this)[uniform], x, y, z); return *this;
	}
	Shader& setUniform(const std::string& uniform, int x, int y, int z, int w) {
		glUniform4i((*this)[uniform], x, y, z, w); return *this;
	}
	Shader& setUniform(const std::string& uniform, float x, float y, float z, float w) {
		glUniform4f((*this)[uniform], x, y, z, w); return *this;
	}

	// Some operators
	operator bool() const { return program != 0; }
	bool operator==(const Shader& rhs) const { return program == rhs.program; }
	bool operator!=(const Shader& rhs) const { return program != rhs.program; }

	GLuint program, vert_shader, frag_shader; ///< shader object ids

	/** Returns pointer to the currently used shader. */
	static Shader* current() {
		GLint i;
		glGetIntegerv(GL_CURRENT_PROGRAM, &i);
		return shaders[i];
	}

  private:
	int gl_response;

	typedef std::map<std::string, GLint> UniformMap;
	UniformMap uniforms; ///< Cached uniform locations, use operator[] to access

	typedef std::map<GLint, Shader*> ShaderMap;
	static ShaderMap shaders; ///< Shader objects for reverse look-up by id
};


/** Temporarily switch shader in a RAII manner. */
struct UseShader {
	UseShader(Shader& new_shader): m_shader(new_shader) {
		glGetIntegerv(GL_CURRENT_PROGRAM, &m_old);
		m_shader.bind();
	}
	~UseShader() { glUseProgram(m_old); }
	/// Access the bound shader
	Shader& operator()() { return m_shader; }

  private:
	Shader& m_shader;
	GLint m_old;
};


/** Load shaders. */
void loadShaders();
