#include "glutil.hh"

namespace glutil {

void VertexArray::Draw(GLint mode) {
	if (empty()) return;
	unsigned stride = sizeof(VertexInfo);
	GLint program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &program);
	GLint vertPos = glGetAttribLocation(program, "vertPos");
	GLint vertTexCoord = glGetAttribLocation(program, "vertTexCoord");
	GLint vertNormal = glGetAttribLocation(program, "vertNormal");
	GLint vertColor = glGetAttribLocation(program, "vertColor");
	if (vertPos != -1) {
		glEnableVertexAttribArray(vertPos);
		glVertexAttribPointer(vertPos, 3, GL_FLOAT, GL_FALSE, stride, &m_vertices[0].position);
	}
	if (vertTexCoord != -1) {
		glEnableVertexAttribArray(vertTexCoord);
		glVertexAttribPointer(vertTexCoord, 2, GL_FLOAT, GL_FALSE, stride, &m_vertices[0].texCoord);
	}
	if (vertNormal != -1) {
		glEnableVertexAttribArray(vertNormal);
		glVertexAttribPointer(vertNormal, 3, GL_FLOAT, GL_FALSE, stride, &m_vertices[0].normal);
	}
	if (vertColor != -1) {
		glEnableVertexAttribArray(vertColor);
		glVertexAttribPointer(vertColor, 4, GL_FLOAT, GL_FALSE, stride, &m_vertices[0].color);
	}
	glDrawArrays(mode, 0, size());

	if (vertPos != -1) glDisableVertexAttribArray(vertPos);
	if (vertTexCoord != -1) glDisableVertexAttribArray(vertTexCoord);
	if (vertNormal != -1) glDisableVertexAttribArray(vertNormal);
	if (vertColor != -1) glDisableVertexAttribArray(vertColor);
}

}
