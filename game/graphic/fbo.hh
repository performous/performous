#pragma once

#include "texture.hh"
#include "graphic/window.hh"

/// Frame Buffer Object class
class FBO {
  public:
	FBO(const FBO&) = delete;
	const FBO& operator=(const FBO&) = delete;
	/// Generate the FBO and attach a fresh texture to it
	FBO(Window& window, float w, float h)
	: m_window(window), m_w(w), m_h(h) {
		// Create FBO
		if (glIsFramebuffer(m_fbo) != GL_TRUE) glGenFramebuffers(1, &m_fbo);
		update();
	}
	/// Handle clean-up
	~FBO() {
		if (glIsFramebuffer(m_fbo))
			glDeleteFramebuffers(1, &m_fbo);
	}
	/// Returns a reference to the attached texture
	OpenGLTexture<GL_TEXTURE_2D>& getTexture() {
		return m_texture;
	}
	/// Bind the FBO into use
	void bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	}
	/// Unbind any FBO
	static void unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void resize(float w, float h) {
		m_w = w;
		m_h = h;
		update();
	}
	float width() const { return m_w; }
	float height() const { return m_h; }
	void update() {
		{
			UseTexture tex(m_window, m_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(m_w), static_cast<GLsizei>(m_h), 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		}
		{
			UseTexture tex(m_window, m_depth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, static_cast<GLsizei>(m_w), static_cast<GLsizei>(m_h), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		}
		// Bind texture as COLOR_ATTACHMENT0
		bind();
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture.id(), 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth.id(), 0);
		unbind();
	}
  private:
	Window& m_window;
	float m_w;
	float m_h;
	GLuint m_fbo {0};
	OpenGLTexture<GL_TEXTURE_2D> m_texture;
	OpenGLTexture<GL_TEXTURE_2D> m_depth;
};

/// RAII FBO binder
struct UseFBO {
	UseFBO(FBO& fbo) { fbo.bind(); }
	~UseFBO() { FBO::unbind(); }
};
