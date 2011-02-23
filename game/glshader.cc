#include "glshader.hh"
#include "glutil.hh"

#include <fstream>
#include <stdexcept>
#include <algorithm>

using namespace glutil;

glmath::Matrix& getColorMatrix() {
	static glmath::Matrix colorMatrix;
	return colorMatrix;
}

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
}

/// Dumps Shader/Program InfoLog
void Shader::dumpInfoLog(GLuint id) {
	int infologLength = 0;
	int maxLength;

	if (glIsShader(id)) glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);
	else glGetProgramiv(id, GL_INFO_LOG_LENGTH, &maxLength);

	char infoLog[maxLength];

	if (glIsShader(id)) glGetShaderInfoLog(id, maxLength, &infologLength, infoLog);
	else glGetProgramInfoLog(id, maxLength, &infologLength, infoLog);

	if (infologLength > 0) {
		std::cout << std::endl << "Errors in shader '" << name << "':\n";
		std::cout << infoLog << std::endl;
	}
}

Shader::Shader(std::string const& name): name(name), program(0) {}

Shader::~Shader() {
	glDeleteProgram(program);
	std::for_each(shader_ids.begin(), shader_ids.end(), glDeleteShader);
	//std::clog << "shader/info: Shader program " << (unsigned)program << " deleted." << std::endl;
}

Shader& Shader::compileFile(std::string const& filename) {
	std::string ext = filename.substr(filename.size() - 5);
	GLenum type;
	if (ext == ".vert") type = GL_VERTEX_SHADER;
	else if (ext == ".geom") type = GL_GEOMETRY_SHADER;
	else if (ext == ".frag") type = GL_FRAGMENT_SHADER;
	else throw std::logic_error("Unknown file extension on shader " + filename);
	std::string srccode = loadFile(filename);
	// Replace "//DEFINES" with defs
	if (!defs.empty()) {
		std::string::size_type pos = srccode.find("//DEFINES");
		if (pos != std::string::npos) srccode = srccode.substr(0, pos) + defs + srccode.substr(pos + 9);
	}
	try {
		return compileCode(srccode, type);
	} catch (std::runtime_error& e) {
		throw std::runtime_error(filename + ": " + e.what());
	}
}


Shader& Shader::compileCode(std::string const& srccode, GLenum type) {
	glutil::GLErrorChecker ec("Shader::compile");
	GLenum new_shader = glCreateShader(type);
	ec.check("glCreateShader");
	char const* source = srccode.c_str();
	glShaderSource(new_shader, 1, &source, NULL);
	ec.check("glShaderSource");

	glCompileShader(new_shader);
	ec.check("glCompileShader");
	glGetShaderiv(new_shader, GL_COMPILE_STATUS, &gl_response);
	dumpInfoLog(new_shader);
	if (gl_response != GL_TRUE) {
		throw std::runtime_error("Shader compile error");
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
	// Attach all compiled shaders to it
	for (ShaderObjects::const_iterator it = shader_ids.begin(); it != shader_ids.end(); ++it)
		glAttachShader(program, *it);
	ec.check("glAttachShader");

	// Link and check status
	glLinkProgram(program);
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
	setUniformMatrix("colorMatrix", getColorMatrix());
	return *this;
}


GLint Shader::operator[](const std::string& uniform) {
	// Try to use a cached value
	UniformMap::iterator it = uniforms.find(uniform);
	if (it != uniforms.end()) return it->second;
	// Get the value and cache it
	GLint var = glGetUniformLocation(program, uniform.c_str());
	if (var == -1) throw std::logic_error("GLSL shader '" + name + "' uniform variable '" + uniform + "' not found.");
	return uniforms[uniform] = var;
}

void VertexArray::Draw(GLint mode) {
	GLint program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &program);
	GLint vertPos = glGetAttribLocation(program, "vertPos");
	GLint vertTexCoord = glGetAttribLocation(program, "vertTexCoord");
	GLint vertNormal = glGetAttribLocation(program, "vertNormal");
	GLint vertColor = glGetAttribLocation(program, "vertColor");
	unsigned vertices = size();
	if (vertPos != -1) {
		glEnableVertexAttribArray(vertPos);
		glVertexAttribPointer(vertPos, 3, GL_FLOAT, GL_FALSE, 0, &m_vertices.front());
	}
	if (vertTexCoord != -1) {
		if (m_texcoords.empty()) m_texcoords.resize(4 * vertices, 0.0f);  // FIXME: Shouldn't be using a texturing shader if not texturing...
		if (m_texcoords.size() != 4 * vertices) throw std::logic_error("Invalid number of vertex texture coordinates");
		glEnableVertexAttribArray(vertTexCoord);
		glVertexAttribPointer(vertTexCoord, 4, GL_FLOAT, GL_FALSE, 0, &m_texcoords.front());
	}
	if (vertNormal != -1) {
		if (m_normals.size() != 3 * vertices) throw std::logic_error("Invalid number of vertex normals");
		glEnableVertexAttribArray(vertNormal);
		glVertexAttribPointer(vertNormal, 3, GL_FLOAT, GL_FALSE, 0, &m_normals.front());
	}
	if (vertColor != 1) {
		if (m_colors.empty()) m_colors.resize(4 * vertices, 1.0f);
		if (m_colors.size() != 4 * vertices) throw std::logic_error("Invalid number of vertex colors");
		glEnableVertexAttribArray(vertColor);
		glVertexAttribPointer(vertColor, 4, GL_FLOAT, GL_FALSE, 0, &m_colors.front());
	}
	glDrawArrays(mode, 0, size());

	if (vertPos != -1) glDisableVertexAttribArray(vertPos);
	if (vertTexCoord != -1) glDisableVertexAttribArray(vertTexCoord);
	if (vertNormal != -1) glDisableVertexAttribArray(vertNormal);
	if (vertColor != -1) glDisableVertexAttribArray(vertColor);
}

