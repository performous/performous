#pragma once

#include <boost/noncopyable.hpp>
#include "surface.hh"
#include "video_driver.hh"

/// Frame Buffer Object class
class FBO: public boost::noncopyable {
  public:
	/// Generate the FBO and attach a fresh texture to it
	FBO(): m_texture() {
		{
			UseTexture tex(m_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenW(), screenH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			// When texture area is large, bilinear filter the original
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// The texture wraps over at the edges (repeat)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		// Create FBO
		glGenFramebuffersEXT(1, &m_fbo);
		// Bind texture as COLOR_ATTACHMENT0
		bind();
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_texture.id(), 0);
		unbind();
	}
	/// Handle clean-up
	~FBO() {
		if (m_fbo) glDeleteFramebuffersEXT(1, &m_fbo);
	}
	/// Returns a reference to the attached texture
	OpenGLTexture<GL_TEXTURE_2D>& getTexture() {
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
	OpenGLTexture<GL_TEXTURE_2D> m_texture;
};

/// RAII FBO binder
struct UseFBO {
	UseFBO(FBO& fbo) { fbo.bind(); }
	~UseFBO() { FBO::unbind(); }
};
