#pragma once


#include "color.hh"
#include "glmath.hh"
#include <GL/glew.h>
#include <string>
#include <iostream>
#include <vector>

namespace glutil {

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


