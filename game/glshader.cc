#include "glshader.hh"
#include "glutil.hh"

Shader::Shader(const char* vertex_shader, const char* fragment_shader) {
	vert_shader = glCreateShader(GL_VERTEX_SHADER);
	frag_shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vert_shader, 1, &vertex_shader, NULL);
	glShaderSource(frag_shader, 1, &fragment_shader, NULL);

	glCompileShader(vert_shader);
	glGetShaderiv(vert_shader,GL_COMPILE_STATUS, &gl_response);
	if (gl_response != GL_TRUE) std::cerr << "Something went wrong compiling the vertex shader." << std::endl;

	glCompileShader(frag_shader);
	glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &gl_response);
	if (gl_response != GL_TRUE) std::cerr << "Something went wrong compiling the fragment shader." << std::endl;

	program = glCreateProgram();

	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);

	glLinkProgram(program);
	glUseProgram(program);
}

Shader::~Shader() {
	glDeleteProgram(program);
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
}
