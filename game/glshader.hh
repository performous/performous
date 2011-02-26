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
	// Note: if you reorder or otherwise change the contents of this, VertexArray::Draw() must be modified accordingly
	struct VertexInfo {
		glmath::vec4 position;
		glmath::vec4 texCoord;
		glmath::vec4 normal;
		glmath::vec4 color;
		VertexInfo():
		  position(0.0, 0.0, 0.0, 1.0),
		  texCoord(0.0, 0.0, 0.0, 0.0),
		  normal(0.0, 0.0, 0.0, 0.0),
		  color(1.0, 1.0, 1.0, 1.0)
		{}
	};
	/// handy vertex array capable of drawing itself
	class VertexArray {
	  private:
		std::vector<VertexInfo> m_vertices;
		VertexInfo m_vert;
	  public:
		VertexArray() {}

		VertexArray& Vertex(float x, float y, float z = 0.0f) {
			return Vertex(glmath::vec4(x, y, z, 1.0f));
		}

		VertexArray& Vertex(glmath::vec4 const& v) {
			m_vert.position = v;
			m_vertices.push_back(m_vert);
			m_vert = VertexInfo();
			return *this;
		}

		VertexArray& Normal(float x, float y, float z) {
			return Normal(glmath::vec4(x, y, z, 1.0f));
		}

		VertexArray& Normal(glmath::vec4 const& v) {
			m_vert.normal = v;
			return *this;
		}

		VertexArray& TexCoord(float s, float t, float u = 0.0f, float v = 0.0f) {
			return TexCoord(glmath::vec4(s, t, u, v));
		}

		VertexArray& TexCoord(glmath::vec4 const& v) {
			m_vert.texCoord = v;
			return *this;
		}

		VertexArray& Color(float r, float g, float b, float a = 1.0f) {
			m_vert.color = glmath::vec4(r, g, b, a);
			return *this;
		}

		VertexArray& Color(const glutil::Color& c) {
			return Color(c.r, c.g, c.b, c.a);
		}

		void Draw(GLint mode = GL_TRIANGLE_STRIP);

		bool empty() const {
			return m_vertices.empty();
		}

		unsigned size() const {
			return m_vertices.size();
		}

		void clear() {
			m_vertices.clear();
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

