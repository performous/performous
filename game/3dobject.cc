#include "3dobject.hh"

#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <boost/lexical_cast.hpp>

// TODO: test & fix faces that doesn't have texcoords in the file
// TODO: group handling for loader


namespace {
	static const int HAS_TEXCOORDS = 1;
	static const int HAS_NORMALS = 2;

	/// Returns a word (delimited by delim) in a string st at position pos (1-based)
	std::string getWord(std::string& st, size_t pos, char delim) {
		std::istringstream iss(st);
		std::string ret;
		for (size_t i = 1; i <= pos; i++)
			getline(iss, ret, delim);
		return ret;
	}
}

/// Load a Wavefront .obj file and possibly scale it also
void Object3d::loadWavefrontObj(std::string filepath, float scale) {
	int linenumber = 0;
	std::string row;
	std::ifstream file(filepath.c_str(), std::ios::binary);
	if (!file.is_open()) throw std::runtime_error("Couldn't open object file "+filepath);
	// Get rid of old data
	m_vertices.clear();
	m_faces.clear();
	m_texcoords.clear();
	while (!file.eof()) {
		getline(file, row); // Read a line
		++linenumber;
		std::istringstream srow(row);
		float x,y,z;
		std::string tempst;
		if (row.substr(0,2) == "v ") {  // Vertices
			srow >> tempst >> x >> y >> z;
			m_vertices.push_back(Vertex(x*scale,y*scale,z*scale));
		} else if (row.substr(0,2) == "vt") {  // Texture Coordinates
			srow >> tempst >> x >> y;
			m_texcoords.push_back(TexCoord(x,y));
		} else if (row.substr(0,2) == "vn") {  // Normals
			srow >> tempst >> x >> y >> z;
			double sum = std::abs(x)+std::abs(y)+std::abs(z);
			if (sum == 0) throw std::runtime_error("Invalid normal in "+filepath+":"+boost::lexical_cast<std::string>(linenumber));
			x /= sum; y /= sum; z /= sum; // Normalize components
			m_normals.push_back(Vertex(x,y,z));
		} else if (row.substr(0,2) == "f ") {  // Faces
			Face f;
			srow >> tempst; // Eat away prefix
			// Parse face point's coordinate references
			while (!srow.eof()) {
				std::string fpoint;
				srow >> fpoint;
				for (size_t i = 1; i <= 3; ++i) {
					std::string st_id(getWord(fpoint,i,'/'));
					if (!st_id.empty()) {
						// Vertex indices are 1-based in the file
						int v_id = boost::lexical_cast<int>(st_id) - 1;
						switch (i) {
							case 1: f.vertices.push_back(v_id); break;
							case 2: f.texcoords.push_back(v_id); break;
							case 3: f.normals.push_back(v_id); break;
						}
					}
				}
			}
			// FIXME: We only allow triangle faces since the VBO generator/drawer
			//        cannot handle anything else (at least for now).
			if (f.vertices.size() > 0 && f.vertices.size() != 3)
				throw std::runtime_error("Only triangle faces allowed in "+filepath+":"+boost::lexical_cast<std::string>(linenumber));
			// Face must have equal number of v, vt, vn or none of a kind
			if (f.vertices.size() > 0
			  && (f.texcoords.empty() || (f.texcoords.size() == f.vertices.size()))
			  && (f.normals.empty()   || (f.normals.size() == f.vertices.size()))) {
				m_faces.push_back(f);
			} else {
				throw std::runtime_error("Invalid face in "+filepath+":"+boost::lexical_cast<std::string>(linenumber));
			}
		}
	}
}

void Object3d::drawVBO() {
	UseShader us(getShader("3dobject"));
	int stride = 3*sizeof(GLfloat);
	int offset = 0;
	if (m_vboStructure & HAS_TEXCOORDS) stride += 2*sizeof(GLfloat);
	if (m_vboStructure & HAS_NORMALS) stride += 3*sizeof(GLfloat);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, (const GLvoid *)offset);
	offset += 3*sizeof(GLfloat);

	if (m_vboStructure & HAS_NORMALS) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, stride, (const GLvoid *)offset);
		offset += 3*sizeof(GLfloat);
	}

	if (m_vboStructure & HAS_TEXCOORDS) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, stride, (const GLvoid *)offset);
	}

	glDrawArrays(m_polyType, 0, m_numvertices);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Object3d::generateVBO() {
	std::vector<GLfloat> data;

	if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

	// TODO: We should tessellate everything into triangles somehow
	//       in order to allow non-triangle based meshes.
	m_polyType = GL_TRIANGLES;
	m_numvertices = m_faces.size() * 3; // Triangle faces

	m_vboStructure = 0;
	if (!m_texcoords.empty()) m_vboStructure |= HAS_TEXCOORDS;
	if (!m_normals.empty()) m_vboStructure |= HAS_NORMALS;

	std::vector<Face>::const_iterator i;
	for (i = m_faces.begin(); i != m_faces.end(); ++i) {
		for (size_t j = 0; j < i->vertices.size(); ++j) {
			data.push_back(m_vertices[i->vertices[j]].x);
			data.push_back(m_vertices[i->vertices[j]].y);
			data.push_back(m_vertices[i->vertices[j]].z);

			if (m_vboStructure & HAS_NORMALS) {
				data.push_back(m_normals[i->normals[j]].x);
				data.push_back(m_normals[i->normals[j]].y);
				data.push_back(m_normals[i->normals[j]].z);
			}

			if (m_vboStructure & HAS_TEXCOORDS) {
				data.push_back(m_texcoords[i->texcoords[j]].s);
				data.push_back(m_texcoords[i->texcoords[j]].t);
			}
		}
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*data.size(), &data.front(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	data.clear();
	m_vertices.clear();
	m_normals.clear();
	m_texcoords.clear();
	m_faces.clear();
}
