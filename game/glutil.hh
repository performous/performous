#pragma once


#include "color.hh"
#include "glmath.hh"
#include "glshader.hh"
#include <GL/glew.h>
#include <string>
#include <iostream>
#include <vector>

namespace glutil {

	/// wrapper struct for RAII
	struct PushMatrix {
		PushMatrix() { glPushMatrix(); }
		~PushMatrix() { glPopMatrix(); }
	};

	/// wrapper struct for RAII
	struct PushMatrixMode {
		PushMatrixMode(GLenum mode) { glGetIntegerv(GL_MATRIX_MODE, &m_old); glMatrixMode(mode); glPushMatrix(); }
		~PushMatrixMode() { glPopMatrix(); glMatrixMode(m_old); }
	  private:
		GLint m_old;
	};

	/// wrapper struct for RAII
	struct UseDepthTest {
		/// enable depth test (for 3d objects)
		UseDepthTest() {
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
		}
		~UseDepthTest() {
			glDisable(GL_DEPTH_TEST);
		}
	};

	/// wrapper struct for RAII
	struct Color {
		float r, ///< red component
		      g, ///< green
		      b, ///< blue
		      a; ///< alpha value
		/// create nec Color object from the Color object
		Color(::Color const& c): r(c.r), g(c.g), b(c.b), a(c.a) {
			getColorMatrix() = glmath::Matrix::diagonal(glmath::Vec4(*this));
		//	glColor4fv(*this);
		//	GLfloat ColorVect[] = {r, g, b, a};
		//	glEnableClientState(GL_COLOR_ARRAY);
		//	glColorPointer (4,GL_FLOAT,0,ColorVect);
		}
		Color(glmath::Matrix const& mat): r(), g(), b(), a() {
			getColorMatrix() = mat;
		}
		~Color() {
			r = g = b = a = 1.0f;
			getColorMatrix() = glmath::Matrix::diagonal(glmath::Vec4(*this));
		//	glColor4fv(*this);
		//	glDisableClientState (GL_COLOR_ARRAY);
		}
		/// overload float cast
		operator float*() { return reinterpret_cast<float*>(this); }
		/// overload float const cast
		operator float const*() const { return reinterpret_cast<float const*>(this); }
	};

	/// handy vertex array capable of drawing itself
	class VertexArray {
	  private:
		int m_dimension;
		std::vector<float> m_vertices;
		std::vector<float> m_normals;
		std::vector<float> m_texcoords;
		std::vector<float> m_colors;

	  public:
		VertexArray(): m_dimension(1) {}

		VertexArray& Vertex(float x, float y) {
			m_dimension = 2;
			m_vertices.push_back(x);
			m_vertices.push_back(y);
			return *this;
		}

		VertexArray& Vertex(float x, float y, float z) {
			m_dimension = 3;
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

		VertexArray& TexCoord(float s, float t) {
			m_texcoords.push_back(s);
			m_texcoords.push_back(t);
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

		void Draw(GLint mode = GL_TRIANGLE_STRIP) const {
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(m_dimension, GL_FLOAT, 0, &m_vertices.front());
			if (m_texcoords.size()) {
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 0, &m_texcoords.front());
			}
			if (m_normals.size()) {
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(GL_FLOAT, 0, &m_normals.front());
			}
			if (m_colors.size()) {
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(4, GL_FLOAT, 0, &m_colors.front());
			}
			glDrawArrays(mode, 0, size());

			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		bool empty() const {
			return m_vertices.empty() && m_normals.empty() && m_texcoords.empty() && m_colors.empty();
		}

		unsigned size() const {
			return m_vertices.size() / m_dimension;
		}

		void clear() {
			m_vertices.clear(); m_normals.clear(); m_texcoords.clear(); m_colors.clear();
		}

		std::vector<float>& getVertices() { return m_vertices; }
		std::vector<float>& getNormals() { return m_normals; }
		std::vector<float>& getTexCoords() { return m_texcoords; }
		std::vector<float>& getColors() { return m_colors; }
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

	/// Checks for OpenGL error and displays it with given location info
	struct GLErrorChecker {
		std::string info;
		GLErrorChecker(std::string const& info): info(info) { check("precondition"); }
		~GLErrorChecker() { check("postcondition"); }
		void check(std::string const& what = "check()") {
			GLenum err = glGetError();
			if (err == GL_NO_ERROR) return;
			std::clog << "opengl/error: " << msg(err) << " in " << info << " " << what << std::endl;
		}
		static void reset() { glGetError(); }
		static std::string msg(GLenum err) {
			switch(err) {
				case GL_NO_ERROR: return std::string();
				case GL_INVALID_ENUM: return "Invalid enum";
				case GL_INVALID_VALUE: return "Invalid value";
				case GL_INVALID_OPERATION: return "Invalid operation";
				case GL_INVALID_FRAMEBUFFER_OPERATION: return "FBO is not complete";
				case GL_STACK_OVERFLOW: return "Stack overflow";
				case GL_STACK_UNDERFLOW: return "Stack underflow";
				case GL_OUT_OF_MEMORY: return "Out of memory";
				default: return "Unknown error";
			}
		}
	};
}


