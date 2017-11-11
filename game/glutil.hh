#pragma once


#include "color.hh"
#include "glmath.hh"
#include <epoxy/gl.h>
#include <string>
#include <iostream>
#include <vector>

namespace glutil {

	// Note: if you reorder or otherwise change the contents of this, VertexArray::Draw() must be modified accordingly
	struct VertexInfo {
		glmath::vec3 vertPos;
		glmath::vec2 vertTexCoord;
		glmath::vec3 vertNormal;
		glmath::vec4 vertColor;
		VertexInfo():
		  vertPos(0.0, 0.0, 0.0),
		  vertTexCoord(0.0, 0.0),
		  vertNormal(0.0, 0.0, 0.0),
		  vertColor(1.0, 1.0, 1.0, 1.0)
		{}
	};
	
	enum VBOTarget { 
		VBO_SURFACE = 0,
		VBO_MODELS = 1
	};

	/// Handy vertex array capable of drawing itself
	class VertexArray {
	  private:
		std::vector<VertexInfo> m_vertices;
		VertexInfo m_vert;
	  public:
		VertexArray() { }

		~VertexArray() { clear(); }

		VertexArray& vertex(float x, float y, float z = 0.0f) {
			return vertex(glmath::vec3(x, y, z));
		}

		VertexArray& vertex(glmath::vec3 const& v) {
			m_vert.vertPos = v;
			m_vertices.push_back(m_vert);
			m_vert = VertexInfo();
			return *this;
		}

		VertexArray& normal(float x, float y, float z) {
			return normal(glmath::vec3(x, y, z));
		}

		VertexArray& normal(glmath::vec3 const& v) {
			m_vert.vertNormal = v;
			return *this;
		}

		VertexArray& texCoord(float s, float t) {
			return texCoord(glmath::vec2(s, t));
		}

		VertexArray& texCoord(glmath::vec2 const& v) {
			m_vert.vertTexCoord = v;
			return *this;
		}

		VertexArray& color(glmath::vec4 const& v) {
			m_vert.vertColor = v;
			return *this;
		}

		void draw(GLEnum mode = GL_TRIANGLE_STRIP);

		bool empty() const {
			return m_vertices.empty();
		}

		GLsizei size() const {
			return m_vertices.size();
		}
		
	  	static GLsizei stride() { return sizeof(VertexInfo); }

		void clear() {
			m_vertices.clear();
			glBindBuffer(GL_ARRAY_BUFFER, 0);
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

	/// Wrapper struct for RAII
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
}


