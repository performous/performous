#include "3dobject.hh"
#include "surface.hh"
#include "glutil.hh"

#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <string>
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


void Object3d::loadWavefrontObj(std::string filepath, float scale) {
	std::string row;
	std::ifstream file(filepath.c_str(), std::ios::binary);
	if (!file.is_open()) throw std::runtime_error("Couldn't open object file "+filepath);
	m_vertices.clear();
	m_faces.clear();
	m_texcoords.clear();
	while (!file.eof()) {
		getline(file, row);
		std::istringstream srow(row);
		float x,y,z;
		std::string tempst;
		if (row.substr(0,2) == "v ") {			// Vertices
			srow >> tempst >> x >> y >> z;
			m_vertices.push_back(Vertex(x*scale,y*scale,z*scale));
		} else if (row.substr(0,2) == "vt") {		// Texture Coordinates
			srow >> tempst >> x >> y;
			m_texcoords.push_back(TexCoord(x,y));
		} else if (row.substr(0,2) == "vn") {		// Normals
			srow >> tempst >> x >> y >> z;
			double sum = std::abs(x)+std::abs(y)+std::abs(z);
			if (sum == 0) throw std::runtime_error("Object "+filepath+" has invalid normal(s).");
			x /= sum; y /= sum; z /= sum;
			m_normals.push_back(Vertex(x,y,z));
		} else if (row.substr(0,2) == "f ") {		// Faces
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


void Object3d::generateDisplayList() {
	if (m_displist != 0) glDeleteLists(m_displist, 1);
	m_displist = glGenLists(1);
	glutil::DisplayList displist(m_displist, GL_COMPILE);
	std::vector<Face>::const_iterator it;
	// Iterate through faces
	for (it = m_faces.begin(); it != m_faces.end(); it++) {
		// Select a suitable primitive
		GLenum polyType = GL_POLYGON;
		switch (it->vertices.size()) {
			case 3: polyType = GL_TRIANGLES; break;
			case 4: polyType = GL_QUADS; break;
		}
		glutil::Begin block(polyType);
		// Iterate through face's points
		std::vector<int>::const_iterator it2;
		for (size_t i = 0; i < it->vertices.size(); i++) {
			// Texture coordinates
			if (!it->texcoords.empty())
			  glTexCoord2f(m_texcoords[it->texcoords[i]].s, m_texcoords[it->texcoords[i]].t);
			// Normals
			if (!it->normals.empty())
			  glNormal3f(
				m_normals[it->normals[i]].x,
				m_normals[it->normals[i]].y,
				m_normals[it->normals[i]].z);
			// Vertices
			glVertex3f(
			  m_vertices[it->vertices[i]].x,
			  m_vertices[it->vertices[i]].y,
			  m_vertices[it->vertices[i]].z);
		}
	}
}
