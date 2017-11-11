#include "glutil.hh"
#include "screen.hh"

namespace glutil {

void VertexArray::draw(GLenum mode) {
	Game* gm = Game::getSingletonPtr();
	Window& window = gm->window();
	glutil::GLErrorChecker glerror("VertexArray::draw");
	if (empty()) return;
	glBindVertexArray(window.VAO());
	glBindBuffer(GL_ARRAY_BUFFER, window.VBO());
	glBufferData(GL_ARRAY_BUFFER, stride() * size(), &m_vertices.front(), (mode == GL_TRIANGLES) ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);

	std::clog << "opengl/debug: Will draw " << size() << " vertices now..." << std::endl;	
// 	std::clog << "opengl/debug: VertexInfo size: " << size() << std::endl;
// 	int vertexNum = 0;
// 	for (auto const& vertex: m_vertices) {
// 	std::clog << "opengl/debug: Vertex at index: " << vertexNum;
// 		std::clog << ", Position: " << &vertex.vertPos;
// 		std::clog << ", TexCoord: " << &vertex.vertTexCoord;
// 		std::clog << ", Normal: " << &vertex.vertNormal;
// 		std::clog << ", Color: " << &vertex.vertColor;
// 		std::clog << std::endl;
// 		vertexNum++;
// 	}
	glerror.check("VertexAttrib::draw, Before draw...");
	{
	glDrawArrays(mode, 0, size());
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

}
