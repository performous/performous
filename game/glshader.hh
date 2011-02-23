#pragma once

#include "glmath.hh"
#include "glutil.hh"
#include <string>
#include <map>
#include <vector>
#include <GL/glew.h>
#include <boost/noncopyable.hpp>

struct Shader: public boost::noncopyable {
	/// Print compile errors and such
	/// @param id of shader or program
	void dumpInfoLog(GLuint id);

	Shader(std::string const& name);
	~Shader();
	/// Set a string that will replace "//DEFINES" in anything loaded by compileFile
	Shader& setDefines(std::string const& defines) { defs = defines; return *this; }
	/// Load shader from file
	Shader& compileFile(std::string const& filename);
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

namespace glutil {
	/// handy vertex array capable of drawing itself
	class VertexArray {
	  private:
		std::vector<float> m_vertices;
		std::vector<float> m_normals;
		std::vector<float> m_texcoords;
		std::vector<float> m_colors;

	  public:
		VertexArray() {}

		VertexArray& Vertex(float x, float y, float z = 0.0f) {
			m_vertices.push_back(x);
			m_vertices.push_back(y);
			m_vertices.push_back(z);
			return *this;
		}

		VertexArray& Normal(float x, float y, float z) {
			m_normals.push_back(x);
			m_normals.push_back(y);
			m_normals.push_back(z);
			return *this;
		}

		VertexArray& TexCoord(float s, float t, float u = 0.0f, float v = 0.0f) {
			m_texcoords.push_back(s);
			m_texcoords.push_back(t);
			m_texcoords.push_back(u);
			m_texcoords.push_back(v);
			return *this;
		}

		VertexArray& Color(float r, float g, float b, float a) {
			m_colors.push_back(r);
			m_colors.push_back(g);
			m_colors.push_back(b);
			m_colors.push_back(a);
			return *this;
		}

		VertexArray& Color(const glutil::Color& c) {
			m_colors.push_back(c.r);
			m_colors.push_back(c.g);
			m_colors.push_back(c.b);
			m_colors.push_back(c.a);
			return *this;
		}

		void Draw(GLint mode = GL_TRIANGLE_STRIP);

		bool empty() const {
			return m_vertices.empty() && m_normals.empty() && m_texcoords.empty() && m_colors.empty();
		}

		unsigned size() const {
			return m_vertices.size() / 3;
		}

		void clear() {
			m_vertices.clear(); m_normals.clear(); m_texcoords.clear(); m_colors.clear();
		}

	};

	/// easy line
	struct Line {
		Line(float x1, float y1, float x2, float y2) {
			VertexArray va;
			va.Vertex(x1, y1);
			va.Vertex(x2, y2);
			va.Draw(GL_LINES);
		}
	};

	/// easy square
	struct Square {
		Square(float cx, float cy, float r, bool filled = false) {
			VertexArray va;
			if (filled) {
				va.Vertex(cx - r, cy + r);
				va.Vertex(cx + r, cy + r);
				va.Vertex(cx - r, cy - r);
				va.Vertex(cx + r, cy - r);
				va.Draw(GL_TRIANGLE_STRIP);
			} else {
				va.Vertex(cx - r, cy + r);
				va.Vertex(cx + r, cy + r);
				va.Vertex(cx + r, cy - r);
				va.Vertex(cx - r, cy - r);
				va.Draw(GL_LINE_LOOP);
			}
		}
	};

}

