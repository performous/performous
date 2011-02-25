#pragma once

#include <string>
#include <vector>
#include <iostream>

#include <GL/glew.h>

#include "color.hh"
#include "glmath.hh"

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
	struct Begin {
		/// call glBegin with given mode
		Begin(GLint mode) { glBegin(mode); }
		~Begin() { glEnd(); }
	};

	/// wrapper struct for RAII
	struct DisplayList {
		/// call glNewList with given list id and mode
		DisplayList(GLuint id, GLenum mode) { glNewList(id, mode); }
		~DisplayList() { glEndList(); }
	};

	/// wrapper struct for RAII
	struct UseLighting {
		/// enable lighting and depth test for 3d objects
		UseLighting(bool doit = true) {
			if (doit) {
				glClear(GL_DEPTH_BUFFER_BIT);
				glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

				GLfloat light_position[] = { -50.0, 15.0, -5.0, 1.0 };
				glLightfv(GL_LIGHT0, GL_POSITION, light_position);

				glEnable(GL_DEPTH_TEST);
				glEnable(GL_LIGHTING);
				glEnable(GL_COLOR_MATERIAL);
				glEnable(GL_LIGHT0);
			}
		}
		~UseLighting() {
			glDisable(GL_LIGHT0);
			glDisable(GL_COLOR_MATERIAL);
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
		}
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

	struct Color {
		float r, ///< red component
		      g, ///< green
		      b, ///< blue
		      a; ///< alpha value
		/// create nec Color object from the Color object
		Color(::Color const& c): r(c.r), g(c.g), b(c.b), a(c.a) {
			glColor4fv(*this);
		}
		Color(glmath::Matrix const& mat): r(), g(), b(), a() {
			// TODO: FIXME
		}
		~Color() {
			r = g = b = a = 1.0f;
			glColor4fv(*this);
		//	glDisableClientState (GL_COLOR_ARRAY);
		}
		/// overload float cast
		operator float*() { return reinterpret_cast<float*>(this); }
		/// overload float const cast
		operator float const*() const { return reinterpret_cast<float const*>(this); }
	};

	// Note: if you reorder or otherwise change the contents of this, VertexArray::Draw() must be modified accordingly
	struct VertexInfo {
		glmath::Vec4 position;
		glmath::Vec4 texCoord;
		glmath::Vec4 normal;
		glmath::Vec4 color;
		bool has_texCoord;
		bool has_normal;
		bool has_color;
		VertexInfo():
		  position(0.0, 0.0, 0.0, 1.0),
		  texCoord(0.0, 0.0, 0.0, 0.0),
		  normal(0.0, 0.0, 0.0, 0.0),
		  color(1.0, 1.0, 1.0, 1.0),
		  has_texCoord(),
		  has_normal(),
		  has_color()
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
			return Vertex(glmath::Vec4(x, y, z, 1.0f));
		}

		VertexArray& Vertex(glmath::Vec4 const& v) {
			m_vert.position = v;
			m_vertices.push_back(m_vert);
			m_vert = VertexInfo();
			return *this;
		}

		VertexArray& Normal(float x, float y, float z) {
			return Normal(glmath::Vec4(x, y, z, 1.0f));
		}

		VertexArray& Normal(glmath::Vec4 const& v) {
			m_vert.normal = v;
			m_vert.has_normal = true;
			return *this;
		}

		VertexArray& TexCoord(float s, float t, float u = 0.0f, float v = 0.0f) {
			return TexCoord(glmath::Vec4(s, t, u, v));
		}

		VertexArray& TexCoord(glmath::Vec4 const& v) {
			m_vert.texCoord = v;
			m_vert.has_texCoord = true;
			return *this;
		}

		VertexArray& Color(float r, float g, float b, float a = 1.0f) {
			m_vert.color = glmath::Vec4(r, g, b, a);
			m_vert.has_color = true;
			return *this;
		}

		VertexArray& Color(const glutil::Color& c) {
			return Color(c.r, c.g, c.b, c.a);
		}

		void Draw(GLint mode = GL_TRIANGLE_STRIP) {
			glutil::Begin block(mode);
			for(std::vector<VertexInfo>::const_iterator it = m_vertices.begin() ; it != m_vertices.end() ; ++it) {
				//glTexCoord4f(it->texCoord[0], it->texCoord[1], it->texCoord[2], it->texCoord[3]);
				if(it->has_texCoord)
					glTexCoord2f(it->texCoord[0], it->texCoord[1]);
				if(it->has_normal)
					glNormal3f(it->normal[0], it->normal[1], it->normal[2]);
				if(it->has_color)
					glColor4f(it->color[0], it->color[1], it->color[2], it->color[3]);
				glVertex3f(it->position[0], it->position[1], it->position[2]);
			}
		}

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

