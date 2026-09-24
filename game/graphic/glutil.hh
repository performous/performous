#pragma once


#include "../color.hh"
#include "glmath.hh"
#include <epoxy/gl.h>
#include <string>
#include <iostream>
#include <vector>

namespace glutil {

	GLintptr alignOffset(GLintptr offset);

	// Note: if you reorder or otherwise change the contents of this, VertexArray::Draw() must be modified accordingly
	struct VertexInfo {
		glmath::vec3 vertPos = glmath::vec3(0.0f);
		glmath::vec2 vertTexCoord = glmath::vec2(0.0f);
		glmath::vec3 vertNormal = glmath::vec3(0.0f);
		glmath::vec4 vertColor = glmath::vec4(1.0f);
	};
	
	// Uniform block structs
	struct shaderMatrices {
		glmath::mat4 projMatrix; // 0 --- Equals vec4[4].
		glmath::mat4 mvMatrix; // 64 --- Equals vec4[4].
		glmath::mat4 normalMatrix; // 128 --- Equals vec4[4], but this one should be converted to mat3 in the shader.
		glmath::mat4 colorMatrix; // 192 --- Equals vec4[4].
		
		static GLsizeiptr size() { return sizeof(shaderMatrices); };
		static GLintptr offset() { return 0; };
		shaderMatrices() {};
		shaderMatrices(const shaderMatrices&) = delete;
		shaderMatrices& operator=(const shaderMatrices&) = delete;
	}; // 256 bytes
	
	struct stereo3dParams {
		float sepFactor; // 256
		float z0; // 260
		float s3dPadding[2] = {7.0f, 13.0f}; // 264
		
		static GLsizeiptr size() { return sizeof(stereo3dParams); };
		static GLintptr offset() { return alignOffset(shaderMatrices::size()); };
		stereo3dParams() {};
		stereo3dParams(const stereo3dParams&) = delete;
		stereo3dParams& operator=(const stereo3dParams&) = delete;
	}; // 16 bytes
	
	struct lyricColorUniforms {
		glmath::vec4 origFill; // 272
		glmath::vec4 origStroke; // 288
		glmath::vec4 newFill; // 304
		glmath::vec4 newStroke; // 320

		static GLsizeiptr size() { return sizeof(lyricColorUniforms); };
		static GLintptr offset() { return alignOffset(stereo3dParams::offset() + stereo3dParams::size()); };
		lyricColorUniforms() {};
		lyricColorUniforms(const lyricColorUniforms&) = delete;
		lyricColorUniforms& operator=(const lyricColorUniforms&) = delete;
	}; // 64 bytes
	
	struct danceNoteUniforms {
		int noteType; // 336
		float hitAnim; // 340
		float clock; // 344
		float scale; // 348
		glmath::vec2 position; // 352
		float dnPadding[2] = {7.0f, 13.0f};

		static GLsizeiptr size() { return sizeof(danceNoteUniforms); };
		static GLintptr offset() { return alignOffset(lyricColorUniforms::offset() + lyricColorUniforms::size()); };
		danceNoteUniforms() {};
		danceNoteUniforms(const danceNoteUniforms&) = delete;
		danceNoteUniforms& operator=(const danceNoteUniforms&) = delete;
	}; // 32 bytes
	// Total 368 bytes

	/// Handy vertex array capable of drawing itself
	class VertexArray {
	private:
		std::vector<VertexInfo> m_vertices;
		VertexInfo m_vert;
	public:
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

		void draw(GLint mode = GL_TRIANGLE_STRIP);

		bool empty() const {
			return m_vertices.empty();
		}

		GLsizei size() const {
			return static_cast<int>(m_vertices.size());
		}
		
		static GLsizei stride() { return sizeof(VertexInfo); }
		
		void clear();
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

	/// Checks for OpenGL error and displays it with given location info
	class GLErrorChecker {
		static thread_local std::vector<std::string> stack;
		std::string info;
		void setWhat(std::string what);
	public:
		GLErrorChecker(std::string const& info);
		~GLErrorChecker();
		void check(std::string const& what = "check()");  ///< An error-check milestone; will log and  any active GL errors
		static void reset() { glGetError(); }  ///< Ignore any existing error
		static std::string msg(GLenum err);
	};
}


