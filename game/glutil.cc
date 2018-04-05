#include "glutil.hh"

namespace glutil {

VertexArray::VertexArray() {
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
}

VertexArray::~VertexArray() {
	clear();
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
}

void VertexArray::clear() {
	m_vertices.clear();
}

void VertexArray::draw(GLint mode) {
	GLErrorChecker glerror("VertexArray::draw");
	if (empty()) return;
	unsigned stride = sizeof(VertexInfo);
	GLint program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &program);
	GLint vertPos = glGetAttribLocation(program, "vertPos");
	GLint vertTexCoord = glGetAttribLocation(program, "vertTexCoord");
	GLint vertNormal = glGetAttribLocation(program, "vertNormal");
	GLint vertColor = glGetAttribLocation(program, "vertColor");
	glerror.check("program and attribs");
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexInfo) * size(), &m_vertices.front(), GL_STATIC_DRAW);
	glerror.check("bind");
	if (vertPos != -1) {
		glEnableVertexAttribArray(vertPos);
		const GLvoid* ptr = m_vbo ? (GLvoid*)offsetof(VertexInfo, position) : &m_vertices[0].position;
		glVertexAttribPointer(vertPos, 3, GL_FLOAT, GL_FALSE, stride, ptr);
	}
	if (vertTexCoord != -1) {
		const GLvoid* ptr = m_vbo ? (GLvoid*)offsetof(VertexInfo, texCoord) : &m_vertices[0].texCoord;
		glEnableVertexAttribArray(vertTexCoord);
		glVertexAttribPointer(vertTexCoord, 2, GL_FLOAT, GL_FALSE, stride, ptr);
	}
	if (vertNormal != -1) {
		const GLvoid* ptr = m_vbo ? (GLvoid*)offsetof(VertexInfo, normal) : &m_vertices[0].normal;
		glEnableVertexAttribArray(vertNormal);
		glVertexAttribPointer(vertNormal, 3, GL_FLOAT, GL_FALSE, stride, ptr);
	}
	if (vertColor != -1) {
		const GLvoid* ptr = m_vbo ? (GLvoid*)offsetof(VertexInfo, color) : &m_vertices[0].color;
		glEnableVertexAttribArray(vertColor);
		glVertexAttribPointer(vertColor, 4, GL_FLOAT, GL_FALSE, stride, ptr);
	}
	glerror.check("enable arrays");
	glDrawArrays(mode, 0, size());

	if (vertPos != -1) glDisableVertexAttribArray(vertPos);
	if (vertTexCoord != -1) glDisableVertexAttribArray(vertTexCoord);
	if (vertNormal != -1) glDisableVertexAttribArray(vertNormal);
	if (vertColor != -1) glDisableVertexAttribArray(vertColor);
	if (m_vbo) glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

GLErrorChecker::GLErrorChecker(std::string const& info): info(info) {
	stack.push_back(std::string());
	check("before starting");
	stack.back() = info;
}

GLErrorChecker::~GLErrorChecker() { check("after finishing"); stack.pop_back(); }

void GLErrorChecker::check(std::string const& what) {
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		stack.back() = info + " " + what;
		// Prefix with all currently active GLErrorChecker contexts
		std::string logmsg = "opengl/error: ";
		for (auto s: stack) logmsg += s + ": ";
		logmsg += msg(err) + "\n";
		std::clog << logmsg << std::flush;
	}
	stack.back() = info + " after " + what;
}

/* static */ std::string GLErrorChecker::msg(GLenum err) {
	switch(err) {
		case GL_NO_ERROR: return std::string();
		case GL_INVALID_ENUM: return "Invalid enum";
		case GL_INVALID_VALUE: return "Invalid value";
		case GL_INVALID_OPERATION: return "Invalid operation";
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "FBO is not complete";
		case GL_STACK_OVERFLOW: return "Stack overflow";
		case GL_STACK_UNDERFLOW: return "Stack underflow";
		case GL_OUT_OF_MEMORY: return "Out of memory";
		default: return "Unknown error";
	}
}

/* static */ thread_local std::vector<std::string> GLErrorChecker::stack;

}
