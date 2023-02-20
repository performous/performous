#include "shader_manager.hh"

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
