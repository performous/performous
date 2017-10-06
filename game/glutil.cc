#include "glutil.hh"
#include "screen.hh"

namespace glutil {

GLuint GLBuffers::m_vao = 0;
GLuint GLBuffers::m_vbo_ids[5] = { 0 };

void VertexArray::draw(GLint mode, bool OGLText) {
	if (empty()) return;
	if (OGLText != false) std::clog << "opengl/debug: VertexArray::Draw called from OpenGLTexture template." << std::endl;
	glutil::GLErrorChecker glerror("VertexArray::draw.");	
	GLsizei stride = sizeof(VertexInfo);
	GLint program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &program);
	std::clog << "gl/debug: GL_CURRENT_PROGRAM: " << program;
	int vboID = ((mode == GL_TRIANGLES) ? 1 : 0);
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_ids[vboID]);
	glBufferData(GL_ARRAY_BUFFER, stride * size(), &m_vertices[0], (vboID == 1) ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
	std::clog << "opengl/debug: GL_CURRENT_PROGRAM: " << program;
	std::clog << ", vertPos: " << vertPos;
	std::clog << ", vertTexCoord: " << vertTexCoord;
	std::clog << ", vertNormal: " << vertNormal;
	std::clog << ", vertColor: " << vertColor;
	std::clog << ", stride: " << stride;
	std::clog << ", vbo ID: " << m_vbo_ids[vboID] << std::endl;	
	
	glEnableVertexAttribArray(vertPos);
	glVertexAttribPointer(vertPos, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offsetof(VertexInfo, position));
	
	glEnableVertexAttribArray(vertTexCoord);
	glVertexAttribPointer(vertTexCoord, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offsetof(VertexInfo, texCoord));
	
	glEnableVertexAttribArray(vertNormal);
	glVertexAttribPointer(vertNormal, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offsetof(VertexInfo, normal));
	
	glEnableVertexAttribArray(vertColor);
	glVertexAttribPointer(vertColor, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offsetof(VertexInfo, color));

	glerror.check("VertexAttrib::draw, Before draw...");
	glDrawArrays(mode, 0, size());

	glDisableVertexAttribArray(vertPos);
	glDisableVertexAttribArray(vertTexCoord);
	glDisableVertexAttribArray(vertNormal);
	glDisableVertexAttribArray(vertColor);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

}
