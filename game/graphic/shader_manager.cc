#include "shader_manager.hh"

ShaderManager::ShaderManager(std::shared_ptr<SDL_Window> sdlWindow)
: m_sdlWindow(sdlWindow) {
}

ShaderManager::~ShaderManager() {
	// Careful, Shaders depends on SDL_Window, thus m_shaders need to be
	// destroyed before screen (and thus be creater after)
	resetShaders();
	releaseSDL();
}

Shader& ShaderManager::createOrGet(std::string const& name) {
	auto it = m_shaders.find(name);

	if (it != m_shaders.end()) {
		return *it->second;
	}

	auto kv = std::make_pair(name, std::make_unique<Shader>(name));

	return *m_shaders.insert(std::move(kv)).first->second;
}

void ShaderManager::resetShaders() {
	m_shaders.clear();
}

void ShaderManager::releaseSDL() {
	m_sdlWindow.reset();
}
