#include "3dobject.hh"

#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cmath>


// TODO: test & fix faces that doesn't have texcoords in the file
// TODO: group handling for loader


namespace {
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
	std::string row;
	std::ifstream file(filepath.c_str(), std::ios::binary);
	if (!file.is_open()) throw std::runtime_error("Couldn't open object file "+filepath);
	// Get rid of old data
	m_vertices.clear();
	m_faces.clear();
	m_texcoords.clear();
	while (!file.eof()) {
		getline(file, row); // Read a line
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
			if (sum == 0) throw std::runtime_error("Object "+filepath+" has invalid normal(s).");
			x /= sum; y /= sum; z /= sum; // Normalize components
			m_normals.push_back(Vertex(x,y,z));
		} else if (row.substr(0,2) == "f ") {  // Faces
			Face f;
			srow >> tempst;
			int v_id;
			// Parse face point's coordinate references
			while (!srow.eof()) {
				srow >> tempst;
				for (size_t i = 1; i <= 3; i++) {
					std::string st_id(getWord(tempst,i,'/'));
					if (!st_id.empty()) {
						std::istringstream conv_int(st_id);
						conv_int >> v_id;
						switch (i) {
							// Vertex indices are 1-based in the file
							case 1: f.vertices.push_back(v_id-1); break;
							case 2: f.texcoords.push_back(v_id-1); break;
							case 3: f.normals.push_back(v_id-1); break;
						}
					}
				}
			}
			// Face must have equal number of v, vt, vn or none of a kind
			if (f.vertices.size() > 0
			  && (f.texcoords.empty() || (f.texcoords.size() == f.vertices.size()))
			  && (f.normals.empty()   || (f.normals.size() == f.vertices.size()))) {
				m_faces.push_back(f);
			} else {
				throw std::runtime_error("Object "+filepath+" has invalid face(s).");
			}
		}
	}
}

void Object3d::drawVBO() {
	int stride = 3*sizeof(GLfloat);
	int offset = 0;
	if (m_vboStructure & _3DOBJECT_TEXCOORDS) stride += 2*sizeof(GLfloat);
	if (m_vboStructure & _3DOBJECT_NORMALS) stride += 3*sizeof(GLfloat);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, (const GLvoid *)offset);
	offset += 3*sizeof(GLfloat);

	if (m_vboStructure & _3DOBJECT_TEXCOORDS) {
		if (m_texture) UseTexture tex(*m_texture);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, stride, (const GLvoid *)offset);
		offset += 2*sizeof(GLfloat);
	}
	if (m_vboStructure & _3DOBJECT_NORMALS) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, stride, (const GLvoid *)offset);
	}

	glDrawArrays(m_polyType, 0, m_numvertices*stride);

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

	m_numvertices = m_vertices.size();

	m_polyType = GL_POLYGON;
	switch (m_vertices.size()) {
		case 3: m_polyType = GL_TRIANGLES; break;
		case 4: m_polyType = GL_QUADS; break;
	}

	m_vboStructure = 0;
	if (!m_texcoords.empty()) m_vboStructure |= _3DOBJECT_TEXCOORDS;
	if (!m_normals.empty()) m_vboStructure |= _3DOBJECT_NORMALS;

	std::vector<Face>::const_iterator i;
	for (i = m_faces.begin(); i != m_faces.end(); ++i) {
		for (size_t j = 0; j < i->vertices.size(); j++) {
			data.push_back(m_vertices[i->vertices[j]].x);
			data.push_back(m_vertices[i->vertices[j]].y);
			data.push_back(m_vertices[i->vertices[j]].z);

			if (m_vboStructure & _3DOBJECT_TEXCOORDS) {
				data.push_back(m_texcoords[i->texcoords[j]].s);
				data.push_back(m_texcoords[i->texcoords[j]].t);
			}

			if (m_vboStructure & _3DOBJECT_NORMALS) {
				data.push_back(m_normals[i->normals[j]].x);
				data.push_back(m_normals[i->normals[j]].y);
				data.push_back(m_normals[i->normals[j]].z);
			}
		}
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*data.size(), &data.front(), GL_STATIC_DRAW);
	data.clear();
	m_vertices.clear();
	m_normals.clear();
	m_texcoords.clear();
	m_faces.clear();
}
