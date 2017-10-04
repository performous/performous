#include "glutil.hh"

namespace glutil {

void VertexArray::initBuffers() {
	glutil::GLErrorChecker glerror("VertexArray::generateVBO()");
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	glGenBuffers(2, m_vbo_ids);
}

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
		glEnableVertexAttribArray(vertPos);
		const GLvoid* ptr = (GLvoid*)offsetof(VertexInfo, position);
		glVertexAttribPointer(vertPos, 3, GL_FLOAT, GL_FALSE, stride, ptr);
	}
	if (vertTexCoord != -1) {
		glEnableVertexAttribArray(vertTexCoord);
		const GLvoid* ptr = (GLvoid*)offsetof(VertexInfo, texCoord);
		glVertexAttribPointer(vertTexCoord, 2, GL_FLOAT, GL_FALSE, stride, ptr);
	}
	if (vertNormal != -1) {
		glEnableVertexAttribArray(vertNormal);
		const GLvoid* ptr = (GLvoid*)offsetof(VertexInfo, normal);
		glVertexAttribPointer(vertNormal, 3, GL_FLOAT, GL_FALSE, stride, ptr);
	}
	if (vertColor != -1) {
		glEnableVertexAttribArray(vertColor);
		const GLvoid* ptr = (GLvoid*)offsetof(VertexInfo, color);
		glVertexAttribPointer(vertColor, 4, GL_FLOAT, GL_FALSE, stride, ptr);
	}
// 	std::clog << "opengl/debug: VertexInfo size: " << size() << std::endl;
// 	int vertexNum = 0;
// 	for (auto const& vertex: m_vertices) {
// 	std::clog << "opengl/debug: Vertex at index: " << vertexNum;
// 		std::clog << ", Position: " << &vertex.position;
// 		std::clog << ", TexCoord: " << &vertex.texCoord;
// 		std::clog << ", Normal: " << &vertex.normal;
// 		std::clog << ", Color: " << &vertex.color;
// 		std::clog << std::endl;
// 		vertexNum++;
// 	}
	glerror.check("VertexAttrib::draw, Before draw...");
	glDrawArrays(mode, 0, size());

	if (vertPos != -1) glDisableVertexAttribArray(vertPos);
	if (vertTexCoord != -1) glDisableVertexAttribArray(vertTexCoord);
	if (vertNormal != -1) glDisableVertexAttribArray(vertNormal);
	if (vertColor != -1) glDisableVertexAttribArray(vertColor);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

}
