#include "glutil.hh"

namespace glutil {

void VertexArray::generateVBO() {
	glutil::GLErrorChecker glerror("VertexArray::generateVBO()");
	glGenBuffers(2, m_vbo_ids);
}

void VertexArray::draw(GLint mode, bool OGLText) {
	if (empty()) return;
	if (OGLText != false) std::clog << "opengl/debug: VertexArray::Draw called from OpenGLTexture template." << std::endl;

	GLsizei stride = sizeof(VertexInfo);
	GLint program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &program);
	std::clog << "gl/debug: GL_CURRENT_PROGRAM: " << program;
	int vboID = ((mode == GL_TRIANGLES) ? 1 : 0);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_ids[vboID]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexInfo) * size(), &m_vertices.front(), (vboID == 1) ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
	std::clog << "opengl/debug: GL_CURRENT_PROGRAM: " << program;
	GLint vertPos = glGetAttribLocation(program, "vertPos");
	std::clog << ", vertPos: " << vertPos;
	GLint vertTexCoord = glGetAttribLocation(program, "vertTexCoord");
	std::clog << ", vertTexCoord: " << vertTexCoord;
	GLint vertNormal = glGetAttribLocation(program, "vertNormal");
	std::clog << ", vertNormal: " << vertNormal;
	GLint vertColor = glGetAttribLocation(program, "vertColor");
	std::clog << ", vertColor: " << vertColor;
	std::clog << ", stride: " << stride;
	std::clog << ", vbo ID: " << m_vbo_ids[vboID] << std::endl;	
	if (vertPos != -1) {
		const GLvoid* ptr = (GLvoid*)offsetof(VertexInfo, position);
		glEnableVertexAttribArray(vertPos);
		glVertexAttribPointer(vertPos, 3, GL_FLOAT, GL_FALSE, stride, ptr);
	}
	if (vertTexCoord != -1) {
		const GLvoid* ptr = (GLvoid*)offsetof(VertexInfo, texCoord);
		glEnableVertexAttribArray(vertTexCoord);
		glVertexAttribPointer(vertTexCoord, 2, GL_FLOAT, GL_FALSE, stride, ptr);
	}
	if (vertNormal != -1) {
		const GLvoid* ptr = (GLvoid*)offsetof(VertexInfo, normal);
		glEnableVertexAttribArray(vertNormal);
		glVertexAttribPointer(vertNormal, 3, GL_FLOAT, GL_FALSE, stride, ptr);
	}
	if (vertColor != -1) {
		const GLvoid* ptr = (GLvoid*)offsetof(VertexInfo, color);
		glEnableVertexAttribArray(vertColor);
		glVertexAttribPointer(vertColor, 4, GL_FLOAT, GL_FALSE, stride, ptr);
	}
	
	glDrawArrays(mode, 0, size());

	if (vertPos != -1) glDisableVertexAttribArray(vertPos);
	if (vertTexCoord != -1) glDisableVertexAttribArray(vertTexCoord);
	if (vertNormal != -1) glDisableVertexAttribArray(vertNormal);
	if (vertColor != -1) glDisableVertexAttribArray(vertColor);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

}
