#pragma once

#include <boost/noncopyable.hpp>
#include "video_driver.hh"

/// Frame Buffer Object class
class FBO: public boost::noncopyable {
  public:
	/// Generate the FBO and attach a fresh texture to it
	FBO(): m_texture() {
		glGenFramebuffersEXT(1, &m_fbo);
		bind();
		{
			UseTexture tex(m_texture);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, screenW(), screenH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			// Bind texture as COLOR_ATTACHMENT0
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, m_texture.id(), 0);
		}
		unbind();
	}
	/// Handle clean-up
	~FBO() {
		if (m_fbo) glDeleteFramebuffersEXT(1, &m_fbo);
	}
	/// Returns a reference to the attached texture
	OpenGLTexture<GL_TEXTURE_RECTANGLE_ARB>& getTexture() {
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
	OpenGLTexture<GL_TEXTURE_RECTANGLE_ARB> m_texture;
};

/// RAII FBO binder
struct UseFBO {
	UseFBO(FBO& fbo) { fbo.bind(); }
	~UseFBO() { FBO::unbind(); }
};
