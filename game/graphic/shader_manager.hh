#pragma once

#include "glshader.hh"

#include <map>
#include <memory>
#include <string>

struct SDL_Window;

class ShaderManager {
  public:
	ShaderManager(std::shared_ptr<SDL_Window>);
	~ShaderManager();

	/// Construct a new shader or return an existing one by name
	Shader& createOrGet(std::string const& name);
	void resetShaders();

  private:
	void releaseSDL();

  private:
	using ShaderMap = std::map<std::string, std::unique_ptr<Shader>>;
	ShaderMap m_shaders; ///< Shader programs by name
	std::shared_ptr<SDL_Window> m_sdlWindow;
};
