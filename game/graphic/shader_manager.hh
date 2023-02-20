#pragma once

#include "glshader.hh"

#include <map>
#include <memory>
#include <string>

class Window;

class ShaderManager {
  public:
	/// Construct a new shader or return an existing one by name
	Shader& createOrGet(std::string const& name);
	void resetShaders();

  private:
	using ShaderMap = std::map<std::string, std::unique_ptr<Shader>>;
	ShaderMap m_shaders; ///< Shader programs by name
};
