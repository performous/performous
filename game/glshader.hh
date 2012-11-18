#pragma once

#include "glmath.hh"
#include "glutil.hh"
#include <string>
#include <map>
#include <vector>
#include <GL/glew.h>
#include <boost/noncopyable.hpp>

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
	void setMat3(GLfloat const* m) { glUniformMatrix3fv(id, 1, GL_FALSE, m); }
	void setMat4(GLfloat const* m) { glUniformMatrix4fv(id, 1, GL_FALSE, m); }
};

struct Shader: public boost::noncopyable {
	/// Print compile errors and such
	/// @param id of shader or program
	void dumpInfoLog(GLuint id);

	Shader(std::string const& name);
	~Shader();
	/// Set a string that will replace "//DEFINES" in anything loaded by compileFile
	Shader& addDefines(std::string const& defines) { defs += defines; return *this; }
	/// Load shader from file
	Shader& compileFile(std::string const& filename);
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

		VertexArray& Color(glmath::vec4 const& v) {
			m_vert.color = v;
			return *this;
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

