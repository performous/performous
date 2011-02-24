#pragma once

#include <boost/noncopyable.hpp>
#include "surface.hh"
#include "video_driver.hh"

/// Frame Buffer Object class
class FBO: boost::noncopyable {
  public:
	/// Generate the FBO and attach a fresh texture to it
	FBO(unsigned w, unsigned h) {
		{
			UseTexture tex(m_texture);
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
		{
			UseTexture tex(m_depth);
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
		}
		// Create FBO
		glGenFramebuffersEXT(1, &m_fbo);
		// Bind texture as COLOR_ATTACHMENT0
		bind();
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, m_texture.id(), 0);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, m_depth.id(), 0);
		unbind();
	}
	/// Handle clean-up
	~FBO() {
		if (m_fbo) glDeleteFramebuffersEXT(1, &m_fbo);
	}
	/// Returns a reference to the attached texture
	OpenGLTexture<GL_TEXTURE_RECTANGLE>& getTexture() {
		return m_texture;
	}
	/// Bind the FBO into use
	void bind() {
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
	}
	/// Unbind any FBO
	static void unbind() {
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

  private:
	GLuint m_fbo;
	OpenGLTexture<GL_TEXTURE_RECTANGLE> m_texture;
	OpenGLTexture<GL_TEXTURE_RECTANGLE> m_depth;
};

/// RAII FBO binder
struct UseFBO {
	UseFBO(FBO& fbo) { fbo.bind(); }
	~UseFBO() { FBO::unbind(); }
};
