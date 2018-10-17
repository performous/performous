#pragma once

#include "fs.hh"
#include "glutil.hh"
#include <string>
#include <map>
#include <vector>
#include <epoxy/gl.h>

struct Uniform {
	GLint id;
	explicit Uniform(GLint id): id(id) {}
	void set(int value) { glUniform1i(id, value); }
	void set(float value) { glUniform1f(id, value); }
	void set(int x, int y) { glUniform2i(id, x, y); }
	void set(float x, float y) { glUniform2f(id, x, y); }
	void set(int x, int y, int z) { glUniform3i(id, x, y, z); }
	void set(float x, float y, float z) { glUniform3f(id, x, y, z); }
	void set(int x, int y, int z, int w) { glUniform4i(id, x, y, z, w); }
	void set(float x, float y, float z, float w) { glUniform4f(id, x, y, z, w); }
	void setMat3(glmath::mat3 const& m) { glUniformMatrix3fv(id, 1, GL_FALSE, &m[0][0]); }
	void setMat4(glmath::mat4 const& m) { glUniformMatrix4fv(id, 1, GL_FALSE, &m[0][0]); }
};

struct Shader {
	Shader(const Shader&) = delete;
  	const Shader& operator=(const Shader&) = delete;
	/// Print compile errors and such
	/// @param id of shader or program
	void dumpInfoLog(GLuint id);

	Shader(std::string const& name);
	~Shader();
	/// Set a string that will replace "//DEFINES" in anything loaded by compileFile
	Shader& addDefines(std::string const& defines) { defs += defines; return *this; }
	/// Load shader from file
	Shader& compileFile(fs::path const& filename);
	/** Compiles a shader of a given type. */
	Shader& compileCode(std::string const& srccode, GLenum type);
	/** Links all compiled shaders to a shader program. */
	Shader& link();

	/** Binds the shader into use. */
	Shader& bind();

	/** Allow setting uniforms in a chain. Shader needs to be in use.*/


	/** Get uniform location. Uses caching internally. */
	Uniform operator[](const std::string& uniform);

	// Some operators
	bool operator==(const Shader& rhs) const { return program == rhs.program; }
	bool operator!=(const Shader& rhs) const { return program != rhs.program; }

private:
	std::string name; ///< for debugging purposes only
	GLuint program; ///< shader program object id
	int gl_response; ///< save last return state

	std::string defs;

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
