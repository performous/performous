#pragma once

#include "shader_manager.hh"

#include <functional>
#include <map>
#include <memory>
#include <SDL_events.h>
#include "glmath.hh"
#include "glshader.hh"
#include "glutil.hh"

struct SDL_Surface;
struct SDL_Window;
class FBO;
class Game;

float screenW();
float screenH();
static inline float virtH() { return float(screenH()) / screenW(); }

/// handles the window
class Window {
	enum class Stereo3dType {RedCyan = 0, GreenMagenta = 1, OverUnder = 2};

  public:
	Window();
	~Window();

	void start();
	void shutdown();

	void render(Game &game, std::function<void (void)> drawFunc);
	/// clears window
	void blank();
	/// Initialize VAO and VBO.
	void initBuffers();
	/// swaps buffers
	void swap();
	/// Handle window events
	void event(Uint8 const& eventID, Sint32 const& data1, Sint32 const& data2);
	/// Resize window (contents) / toggle full screen according to config. Returns true if resized.
	void resize();
	/// take a screenshot
	void screenshot();

	/// Return reference to Uniform Buffer Object.
	static GLuint const& UBO() { return Window::m_ubo; }
	/// Return reference to Vertex Array Object.
	GLuint const& VAO() const { return Window::m_vao; }
	/// Return reference to Vertex Buffer Object.
	GLuint const& VBO() const { return Window::m_vbo; }
	FBO& getFBO();

	/// Store value of GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT.
	static GLint bufferOffsetAlignment;

	/// Construct a new shader or return an existing one by name
	Shader& shader(std::string const& name);
	Shader& getShader(std::string const& name);
	/// Compiles and links all shaders.
	void createShaders();
	void resetShaders();
	void updateColor();
	void updateLyricHighlight(glmath::vec4 const& fill, glmath::vec4 const& stroke, glmath::vec4 const& newFill, glmath::vec4 const& newStroke);
	void updateLyricHighlight(glmath::vec4 const& fill, glmath::vec4 const& stroke);
	void updateTransforms();

  private:
	void setWindowPosition(const Sint32& x, const Sint32& y);
	void setFullscreen();
	/// Setup everything for drawing a view.
	/// @param num 0 = no stereo, 1 = left eye, 2 = right eye
	void view(unsigned num);
	void updateStereo(float separation);

	struct SDLSystem {
		SDLSystem();
		~SDLSystem();
	};

	const GLuint vertPos = 0;
	const GLuint vertTexCoord = 1;
	const GLuint vertNormal = 2;
	const GLuint vertColor = 3;
	bool m_fullscreen = false;
	bool m_needResize = true;
	static GLuint m_ubo;
	static GLuint m_vao;
	static GLuint m_vbo;
	glutil::stereo3dParams m_stereoUniforms;
	glutil::shaderMatrices m_matrixUniforms;
	glutil::lyricColorUniforms m_lyricColorUniforms;
	std::unique_ptr<FBO> m_fbo;
	int m_windowX = 0;
	int m_windowY = 0;

	// Careful, Shaders depends on SDL_Window, thus m_shaders need to be
	// destroyed before screen (and thus be creater after)
	// Keep the sequence of following members

	std::unique_ptr<SDLSystem> m_system;
	std::unique_ptr<SDL_Window, void (*)(SDL_Window*)> screen;
	std::unique_ptr<std::remove_pointer_t<SDL_GLContext> /* SDL_GLContext is a void* */, void (*)(SDL_GLContext)> glContext;
	std::unique_ptr<ShaderManager> m_shaderManager;
};
