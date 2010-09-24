#pragma once

#include <string>
#include <iostream>

#include <GL/glew.h>

#include "color.hh"

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

	/// struct to store color information
	struct Color {
		float r, ///< red component
		      g, ///< green
		      b, ///< blue
		      a; ///< alpha value
		/// create nec Color object with given channels
		Color(float r_ = 0.0, float g_ = 0.0, float b_ = 0.0, float a_ = 1.0): r(r_), g(g_), b(b_), a(a_) {}
		/// create nec Color object from the Color object
		Color(::Color const& c): r(c.r), g(c.g), b(c.b), a(c.a) {}
		/// overload float cast
		operator float*() { return reinterpret_cast<float*>(this); }
		/// overload float const cast
		operator float const*() const { return reinterpret_cast<float const*>(this); }
		/** Apply the color **/
		void operator()() const { glColor4fv(*this); }
		/*
		void operator()() const {
			GLfloat ColorVect[] = {r, g, b, a};
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer (4,GL_FLOAT,0,ColorVect);
			glDisableClientState (GL_COLOR_ARRAY);
		}
		*/
		static void reset() { Color(1.0f, 1.0f, 1.0f)(); }
	};

	struct ColorRIIA {
		float r, ///< red component
		      g, ///< green
		      b, ///< blue
		      a; ///< alpha value
		/// create nec Color object from the Color object
		ColorRIIA(::Color const& c): r(c.r), g(c.g), b(c.b), a(c.a) {
			glColor4fv(*this);
		}
		~ColorRIIA() { r = g = b = a = 1.0f; glColor4fv(*this); }
		/// overload float cast
		operator float*() { return reinterpret_cast<float*>(this); }
		/// overload float const cast
		operator float const*() const { return reinterpret_cast<float const*>(this); }
	};

	/// easy line
	struct Line {
		Line(float x1, float y1, float x2, float y2) {
			Begin line(GL_LINES);
			glVertex2f(x1,y1);
			glVertex2f(x2,y2);
		}
	};

	/// easy square
	struct Square {
		Square(float cx, float cy, float r, bool filled = false) {
			Begin line(filled ? GL_QUADS : GL_LINE_LOOP);
			glVertex2f(cx-r,cy+r);
			glVertex2f(cx-r,cy-r);
			glVertex2f(cx+r,cy-r);
			glVertex2f(cx+r,cy+r);
		}
	};

	/// Checks for OpenGL error and displays it with given location info
	struct GLErrorChecker {
		GLErrorChecker(std::string info = "") {
			GLenum err;
			if ((err = glGetError()) != GL_NO_ERROR) {
				if (!info.empty()) info = " (" + info +")";
				switch(err) {
					case GL_INVALID_ENUM: std::cerr << "OpenGL error: invalid enum" << info << std::endl; break;
					case GL_INVALID_VALUE: std::cerr << "OpenGL error: invalid value" << info << std::endl; break;
					case GL_INVALID_OPERATION: std::cerr << "OpenGL error: invalid operation" << info << std::endl; break;
					case GL_STACK_OVERFLOW: std::cerr << "OpenGL error: stack overflow" << info << std::endl; break;
					case GL_STACK_UNDERFLOW: std::cerr << "OpenGL error: stack underflow" << info << std::endl; break;
					case GL_OUT_OF_MEMORY: std::cerr << "OpenGL error: out of memory" << info << std::endl; break;
				}
			}
		}
	};
}

