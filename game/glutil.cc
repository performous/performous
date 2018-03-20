#include "glutil.hh"

namespace glutil {
	
	void VertexArray::generateVBO() {
		if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
		glGenBuffers(1, &m_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexInfo) * size(), &m_vertices.front(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	void VertexArray::draw(GLint mode) {
		if (empty()) return;
		unsigned stride = sizeof(VertexInfo);
		GLint program;
		glGetIntegerv(GL_CURRENT_PROGRAM, &program);
		GLint vertPos = glGetAttribLocation(program, "vertPos");
		GLint vertTexCoord = glGetAttribLocation(program, "vertTexCoord");
		GLint vertNormal = glGetAttribLocation(program, "vertNormal");
		GLint vertColor = glGetAttribLocation(program, "vertColor");
		if (m_vbo) glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		if (vertPos != -1) {
			const GLvoid* ptr = m_vbo ? (GLvoid*)offsetof(VertexInfo, position) : &m_vertices[0].position;
			glEnableVertexAttribArray(vertPos);
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
		glDrawArrays(mode, 0, size());
		
		if (vertPos != -1) glDisableVertexAttribArray(vertPos);
		if (vertTexCoord != -1) glDisableVertexAttribArray(vertTexCoord);
		if (vertNormal != -1) glDisableVertexAttribArray(vertNormal);
		if (vertColor != -1) glDisableVertexAttribArray(vertColor);
		if (m_vbo) glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
}
