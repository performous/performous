#include "glshader.hh"

#include "glutil.hh"
#include "video_driver.hh"
#include <algorithm>
#include <fstream>
#include <stdexcept>

using namespace glutil;

namespace {
	/// Loads a file into memory
	std::string loadFile(fs::path const& _filepath) {
		auto filepath = _filepath.string();
		std::ifstream f(filepath, std::ios::binary);
		if (!f) throw std::runtime_error(std::string("Couldn't open ") + filepath);
		f.seekg(0, std::ios::end);
		size_t size = f.tellg();
		f.seekg(0);
		std::vector<char> data(size+1); // +1 for terminating null
		if (!f.read(&data[0], size)) throw std::runtime_error(std::string("Unexpected I/O error in ") + filepath);
		data.back() = '\0';
		return std::string(&data[0]);
	}
}

/// Dumps Shader/Program InfoLog
void Shader::dumpInfoLog(GLuint id) {
	GLint maxLength=0;

	if (glIsShader(id)) glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);
	else glGetProgramiv(id, GL_INFO_LOG_LENGTH, &maxLength);

	std::vector<GLchar> infoLog(maxLength);
	int infoLogLength = 0;

	if (glIsShader(id)) glGetShaderInfoLog(id, maxLength, &infoLogLength, infoLog.data());
	else glGetProgramInfoLog(id, maxLength, &infoLogLength, infoLog.data());

	if (maxLength == 0) return;
	// Ignore success messages that the Radeon driver always seems to give
	if (std::equal(infoLog.data(), infoLog.data() + infoLogLength, "Vertex shader(s) linked, fragment shader(s) linked, geometry shader(s) linked.")) return;
	// Format a (possibly multi-line) log message
	std::string prefix = "opengl/error: Shader [" + name + "]: ";
	std::string logmsg = prefix;
	for (char ch: std::string(infoLog.data())) {
		if (logmsg.back() == '\n') logmsg += prefix;
		logmsg += ch;
	}
	if (logmsg.back() != '\n') logmsg += '\n';
	std::clog << logmsg << std::flush;
}

void Shader::bindUniformBlocks() {
	glUseProgram(program);
	glBindBuffer(GL_UNIFORM_BUFFER,Window::UBO());
	GLsizei bufferSize = glutil::danceNoteUniforms::offset() + glutil::danceNoteUniforms::size();
	glBufferData(GL_UNIFORM_BUFFER, bufferSize, NULL, GL_DYNAMIC_DRAW);
	for (std::pair<std::string, unsigned int> const& uniformBlock: Shader::m_uniformblocks) {
			GLuint blockIndex = glGetUniformBlockIndex(program, uniformBlock.first.c_str());
			if (blockIndex != GL_INVALID_INDEX) {
		glutil::GLErrorChecker ec("Shader::bindUniformBlocks");
		ec.check("glBindBufferRange()");
		{
				switch (uniformBlock.second) {
				case 7:
					glBindBufferRange(GL_UNIFORM_BUFFER, 7, Window::UBO(), glutil::shaderMatrices::offset(), sizeof(glutil::shaderMatrices));
					break;
				case 8:
					glBindBufferRange(GL_UNIFORM_BUFFER, 8, Window::UBO(), glutil::stereo3dParams::offset(), sizeof(glutil::stereo3dParams));
					break;
				case 9:
					glBindBufferRange(GL_UNIFORM_BUFFER, 9, Window::UBO(), glutil::lyricColorUniforms::offset(), sizeof(glutil::lyricColorUniforms));
					break;
				case 10:
					glBindBufferRange(GL_UNIFORM_BUFFER, 10, Window::UBO(), glutil::danceNoteUniforms::offset(), sizeof(glutil::danceNoteUniforms));
					break;
				}
			}
		ec.check("glUniformBlockBinding()");
		{
				glUniformBlockBinding(program, blockIndex, uniformBlock.second);
		}
		}
	}
}

Shader::Shader(std::string const& name): name(name), program(0) {}

Shader::~Shader() {
	glDeleteProgram(program);
	std::for_each(shader_ids.begin(), shader_ids.end(), glDeleteShader);
}

Shader& Shader::compileFile(fs::path const& filename) {
	std::clog << "opengl/info: Compiling " << filename.string() << std::endl;
	fs::path ext = filename.extension();
	GLenum type;
	if (ext == ".vert") type = GL_VERTEX_SHADER;
	else if (ext == ".geom") type = GL_GEOMETRY_SHADER;
	else if (ext == ".frag") type = GL_FRAGMENT_SHADER;
	else throw std::logic_error("Unknown file extension on shader " + filename.string());
	std::string srccode = loadFile(filename);
	// Replace "//DEFINES" with defs
	if (!defs.empty()) {
		std::string::size_type pos = srccode.find("//DEFINES");
		if (pos != std::string::npos) srccode = srccode.substr(0, pos) + defs + srccode.substr(pos + 9);
	}
	try {
		return compileCode(srccode, type);
	} catch (std::runtime_error& e) {
		throw std::runtime_error(filename.filename().string() + ": " + e.what());
	}
}

const std::forward_list<std::pair<std::string, unsigned int>> Shader::m_uniformblocks = {
// Holds the block names for our uniform blocks, this list will be iterated on Shader::link to assign valid bindings to each of these.
// Make sure to update this if ever the uniform block names change in GLSL.
	{"shaderMatrices", 7},
	{"stereoParams", 8},
	{"lyricColors", 9},
	{"danceNote", 10}
};


Shader& Shader::compileCode(std::string const& srccode, GLenum type) {
	glutil::GLErrorChecker ec("Shader::compile");
	GLuint new_shader = glCreateShader(type);
	ec.check("glCreateShader");
	if (new_shader == 0) {
		throw std::runtime_error("Couldn't create shader.");
	}
	char const* source = srccode.c_str();
	glShaderSource(new_shader, 1, &source, nullptr);
	ec.check("glShaderSource");

	glCompileShader(new_shader);
	ec.check("glCompileShader");
	glGetShaderiv(new_shader, GL_COMPILE_STATUS, &gl_response);
	dumpInfoLog(new_shader);
	if (gl_response != GL_TRUE) {
		throw std::runtime_error("Shader compile error.");
	}
	
	shader_ids.push_back(new_shader);
	return *this;
}


Shader& Shader::link() {
	glutil::GLErrorChecker ec("Shader::link");
	if (program) throw std::runtime_error("Shader already linked.");
	// Create the program id
	program = glCreateProgram();
	ec.check("glCreateProgram");
	if (program == 0) {
		throw std::runtime_error("Couldn't create shader program.");
	}
	// Attach all compiled shaders to it
	for (auto id : shader_ids) glAttachShader(program, id);
	ec.check("glAttachShader");

	// Link and check status
	glLinkProgram(program);

	// always detach shaders, linked or not, they need to be detached
	for (auto id : shader_ids) glDetachShader(program, id);

	glGetProgramiv(program, GL_LINK_STATUS, &gl_response);
	dumpInfoLog(program);
	if (gl_response != GL_TRUE) {
		throw std::runtime_error("Something went wrong linking the shader program.");
	}
	ec.check("glLinkProgram");

	return *this;
}


Shader& Shader::bind() {
	glutil::GLErrorChecker ec("Shader::bind");
	glUseProgram(program);
	return *this;
}


Uniform Shader::operator[](const std::string& uniform) {
	bind();
	// Try to use a cached value
	auto it = uniforms.find(uniform);
	if (it != uniforms.end()) return Uniform(it->second);
	// Get the value and cache it
	GLint var = glGetUniformLocation(program, uniform.c_str());
	if (var == -1) throw std::logic_error("GLSL shader '" + name + "' uniform variable '" + uniform + "' not found.");
	return Uniform(uniforms[uniform] = var);
}
