#pragma once

#include <string>
#include <map>
#include <vector>
#include <GL/glew.h>
#include <boost/noncopyable.hpp>


struct Shader: public boost::noncopyable {
	/// Print compile errors and such
	/// @param id of shader or program
	static void dumpInfoLog(GLuint id);

	Shader(std::string const& name);
	~Shader();

	/// Load shader from file, add optional defines before the loaded code and compile.
	Shader& compileFile(std::string const& filename, std::string const& defines = std::string());
	/** Compiles a shader of a given type. */
	Shader& compileCode(std::string const& srccode, GLenum type);
	/** Links all compiled shaders to a shader program. */
	Shader& link();

	/** Binds the shader into use. */
	Shader& bind();

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
	Shader& setUniformMatrix(const std::string& uniform, GLfloat const* m) {
		glUniformMatrix4fv((*this)[uniform], 1, GL_FALSE, m); return *this;
	}
	Shader& setUniformMatrix(const std::string& uniform, GLdouble const* m) {
		// Note: need to convert into float because glUniformMatrix4dv is NULL on my machine
		GLfloat arr[16]; std::copy(m, m + 16, arr); return setUniformMatrix(uniform, arr);
	}

	/** Get uniform location. Uses caching internally. */
	GLint operator[](const std::string& uniform);

	// Some operators
	GLuint operator*() { return program; }
	operator GLuint() { return program; }
	operator bool() const { return program != 0; }
	bool operator==(const Shader& rhs) const { return program == rhs.program; }
	bool operator!=(const Shader& rhs) const { return program != rhs.program; }

private:
	std::string name; ///< for debugging purposes only
	GLuint program; ///< shader program object id
	int gl_response; ///< save last return state

	typedef std::vector<GLuint> ShaderObjects;
	ShaderObjects shader_ids;

	typedef std::map<std::string, GLint> UniformMap;
	UniformMap uniforms; ///< Cached uniform locations, use operator[] to access

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
