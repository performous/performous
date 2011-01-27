#include "glshader.hh"
#include "glutil.hh"

#include <fstream>
#include <stdexcept>
#include <algorithm>


namespace {
	/// Loads a file into memory
	std::string loadFile(const std::string& filepath) {
		std::ifstream f(filepath.c_str(), std::ios::binary);
		if (!f.is_open()) throw std::runtime_error(std::string("Couldn't open ") + filepath);
		f.seekg(0, std::ios::end);
		size_t size = f.tellg();
		f.seekg(0);
		std::vector<char> data(size+1); // +1 for terminating null
		if (!f.read(&data[0], size)) throw std::runtime_error(std::string("Unexpected I/O error in ") + filepath);
		data.back() = '\0';
		return std::string(&data[0]);
	}

	/// Dumps Shader/Program InfoLog
	void dumpInfoLog(GLuint id) {
		int infologLength = 0;
		int maxLength;

		if (glIsShader(id)) glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);
		else glGetProgramiv(id, GL_INFO_LOG_LENGTH, &maxLength);

		char infoLog[maxLength];

		if (glIsShader(id)) glGetShaderInfoLog(id, maxLength, &infologLength, infoLog);
		else glGetProgramInfoLog(id, maxLength, &infologLength, infoLog);

		if (infologLength > 0) {
			std::cout << std::endl << "Shader info log:" << std::endl;
			std::cout << infoLog << std::endl;
		}
	}
}

Shader::ShaderMap Shader::shader_progs;

Shader::Shader(): program(0) {}

Shader::Shader(const std::string& vert_path, const std::string& frag_path, bool use): program(0) {
	loadFromFile(vert_path, frag_path, use);
}


Shader::~Shader() {
	shader_progs[program] = NULL;
	glDeleteProgram(program);
	std::for_each(shader_ids.begin(), shader_ids.end(), glDeleteShader);
	//std::clog << "shader/info: Shader program " << (unsigned)program << " deleted." << std::endl;
}


void Shader::loadFromFile(const std::string& vert_path, const std::string& frag_path, bool use) {
	std::string vertstr = loadFile(vert_path);
	std::string fragstr = loadFile(frag_path);
	const char* vert = vertstr.c_str();
	const char* frag = fragstr.c_str();
	compile(vert, GL_VERTEX_SHADER);
	compile(frag, GL_FRAGMENT_SHADER);
	link();
	if (use) bind();
}


void Shader::compile(const char* source, GLenum type) {
	glutil::GLErrorChecker ec("Shader::compile");
	GLenum new_shader = glCreateShader(type);
	ec.check("glCreateShader");
	glShaderSource(new_shader, 1, &source, NULL);
	ec.check("glShaderSource");

	glCompileShader(new_shader);
	glGetShaderiv(new_shader, GL_COMPILE_STATUS, &gl_response);
	if (gl_response != GL_TRUE) {
		dumpInfoLog(new_shader);
		throw std::runtime_error("Something went wrong compiling the shader.");
	}

	shader_ids.push_back(new_shader);
}


void Shader::link() {
	glutil::GLErrorChecker ec("Shader::link");
	if (program) throw std::runtime_error("Shader already linked.");
	// Create the program id
	program = glCreateProgram();
	ec.check("glCreateProgram");
	// Attach all compiled shaders to it
	for (ShaderObjects::const_iterator it = shader_ids.begin(); it != shader_ids.end(); ++it)
		glAttachShader(program, *it);
	ec.check("glAttachShader");

	// Link and check status
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &gl_response);
	if (gl_response != GL_TRUE) {
		dumpInfoLog(program);
		throw std::runtime_error("Something went wrong linking the shader program.");
	}
	ec.check("glLinkProgram");

	shader_progs[program] = this;
}


void Shader::bind() {
	glutil::GLErrorChecker ec("Shader::bind");
	glUseProgram(program);
}


GLint Shader::operator[](const std::string& uniform) {
	// Try to use a cached value
	UniformMap::iterator it = uniforms.find(uniform);
	if (it != uniforms.end()) return it->second;
	// Get the value and cache it
	GLint var = glGetUniformLocation(program, uniform.c_str());
	if (var == -1) throw std::logic_error("GLSL shader uniform variable not found: " + uniform);
	return uniforms[uniform] = var;
}

