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

/// A polygon containing links to required point data
struct Face {
	std::vector<int> vertices;
	std::vector<int> texcoords;
	std::vector<int> normals;
};

/// Load a Wavefront .obj file and possibly scale it also
void Object3d::loadWavefrontObj(std::string filepath, float scale) {
	int linenumber = 0;
	std::string row;
	std::ifstream file(filepath.c_str(), std::ios::binary);
	if (!file.is_open()) throw std::runtime_error("Couldn't open object file "+filepath);
	std::vector<glmath::vec4> m_vertices, m_normals, m_texcoords;
	std::vector<Face> m_faces;
	while (getline(file, row)) {
		++linenumber;
		std::istringstream srow(row);
		float x,y,z;
		std::string tempst;
		if (row.substr(0,2) == "v ") {  // Vertices
			srow >> tempst >> x >> y >> z;
			m_vertices.push_back(glmath::vec4(x*scale, y*scale, z*scale, 1.0f));
		} else if (row.substr(0,2) == "vt") {  // Texture Coordinates
			srow >> tempst >> x >> y;
			m_texcoords.push_back(glmath::vec4(x, y, 0.0f, 0.0f));
		} else if (row.substr(0,2) == "vn") {  // Normals
			srow >> tempst >> x >> y >> z;
			double sum = std::abs(x)+std::abs(y)+std::abs(z);
			if (sum == 0) throw std::runtime_error("Invalid normal in "+filepath+":"+boost::lexical_cast<std::string>(linenumber));
			x /= sum; y /= sum; z /= sum; // Normalize components
			m_normals.push_back(glmath::vec4(x, y, z, 0.0));
		} else if (row.substr(0,2) == "f ") {  // Faces
			Face f;
			srow >> tempst; // Eat away prefix
			// Parse face point's coordinate references
			for (std::string fpoint; srow >> fpoint; ) {
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
	// Construct a vertex array
	for (std::vector<Face>::const_iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
		bool hasNormals = !i->normals.empty();
		bool hasTexCoords = !i->texcoords.empty();
		for (size_t j = 0; j < i->vertices.size(); ++j) {
			if (hasNormals) m_va.Normal(m_normals[i->normals[j]]);
			if (hasTexCoords) m_va.TexCoord(m_texcoords[i->texcoords[j]]);
			m_va.Vertex(m_vertices[i->vertices[j]]);
		}
	}

}

void Object3d::drawVBO() {
	UseShader us(getShader("3dobject"));
	m_va.Draw(GL_TRIANGLES);
}

