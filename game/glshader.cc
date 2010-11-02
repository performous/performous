#include "glshader.hh"
#include "glutil.hh"
#include "video_driver.hh"
#include "3dobject.hh"
#include "fs.hh"

#include <fstream>
#include <stdexcept>


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


Shader::Shader(): program(), vert_shader(), frag_shader() {}

Shader::Shader(const std::string& vert_path, const std::string& frag_path, bool use) {
	loadFromFile(vert_path, frag_path, use);
}


Shader::~Shader() {
	glDeleteProgram(program);
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	//std::clog << "shader/info: Shader program " << (unsigned)program << " deleted." << std::endl;
}


void Shader::loadFromFile(const std::string& vert_path, const std::string& frag_path, bool use) {
	std::string vertstr = loadFile(vert_path);
	std::string fragstr = loadFile(frag_path);
	const char* vert = vertstr.c_str();
	const char* frag = fragstr.c_str();
	loadFromMemory(vert, frag, use);
}


void Shader::loadFromMemory(const char* vert_source, const char* frag_source, bool use) {
	glutil::GLErrorChecker::reset();
	vert_shader = glCreateShader(GL_VERTEX_SHADER);
	frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glutil::GLErrorChecker shadererror("Shader::loadFromMemory - glCreateShader");

	glShaderSource(vert_shader, 1, &vert_source, NULL);
	glShaderSource(frag_shader, 1, &frag_source, NULL);
	glutil::GLErrorChecker shadersourceerror("Shader::loadFromMemory - glShaderSource");

	glCompileShader(vert_shader);
	glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &gl_response);
	if (gl_response != GL_TRUE) {
		dumpInfoLog(vert_shader);
		throw std::runtime_error("Something went wrong compiling the vertex shader.");
	}

	glCompileShader(frag_shader);
	glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &gl_response);
	if (gl_response != GL_TRUE) {
		dumpInfoLog(frag_shader);
		throw std::runtime_error("Something went wrong compiling the fragment shader.");
	}

	glutil::GLErrorChecker::reset();
	program = glCreateProgram();
	glutil::GLErrorChecker createprogramerror("Shader::loadFromMemory - glCreateProgram");

	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glutil::GLErrorChecker attachshadererror("Shader::loadFromMemory - glAttachShader");

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &gl_response);
	if (gl_response != GL_TRUE) {
		dumpInfoLog(program);
		throw std::runtime_error("Something went wrong linking shader program.");
	}
	glutil::GLErrorChecker linkerror("Shader::loadFromMemory - glLinkProgram");

	// Cache uniform locations
	tex = glGetUniformLocation(program, "tex");
	texRect = glGetUniformLocation(program, "texRect");
	texMode = glGetUniformLocation(program, "texMode");
	glutil::GLErrorChecker uniformerror("Shader::loadFromMemory - glGetUniformLocation");

	if (use) bind();
}


void Shader::bind() {
	glUseProgram(program);
	glutil::GLErrorChecker glerror("Shader::bind");
}



void loadShaders() {
	Window::shader.reset(new Shader(getThemePath("shaders/core.vert"), getThemePath("shaders/core.frag"), true));
	Object3d::shader.reset(new Shader(getThemePath("shaders/3dobject.vert"), getThemePath("shaders/3dobject.frag")));
}
