#include "glutil.hh"
#include "video_driver.hh"

namespace glutil {

	GLintptr alignOffset(GLintptr offset) {
		if (Window::bufferOffsetAlignment == -1) {
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &Window::bufferOffsetAlignment);
		}
		float result;
		result = std::ceil(static_cast<float>(offset) / static_cast<float>(Window::bufferOffsetAlignment));
		result *= static_cast<float>(Window::bufferOffsetAlignment);
		return static_cast<GLintptr>(result);
	}

	void VertexArray::clear() {
		m_vertices.clear();
	}

	void VertexArray::draw(GLint mode) {
		GLErrorChecker glerror("VertexArray::draw");
		if (empty()) return;

		glBufferData(GL_ARRAY_BUFFER, stride() * size(), &m_vertices.front(), GL_DYNAMIC_DRAW);

		glerror.check("draw arrays");
		glDrawArrays(mode, 0, size());
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
