#include "3dobject.hh"

#include <fstream>
#include <stdexcept>
#include <sstream>

#include "texture.hh"

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

/// A polygon containing links to required point data
struct Face {
	std::vector<int> vertices;
	std::vector<int> texcoords;
	std::vector<int> normals;
};

/// Load a Wavefront .obj file and possibly scale it also
void Object3d::loadWavefrontObj(fs::path const& filepath, float scale) {
	int linenumber = 0;
	std::string row;
	std::ifstream file(filepath.c_str(), std::ios::binary);
	if (!file) throw std::runtime_error("Couldn't open object file "+filepath.string());
	std::vector<glmath::vec3> vertices;
	std::vector<glmath::vec3> normals;
	std::vector<glmath::vec2> texcoords;
	std::vector<Face> m_faces;
	while (getline(file, row)) {
		++linenumber;
		std::istringstream srow(row);
		float x,y,z;
		std::string tempst;
		if (row.substr(0,2) == "v ") {  // Vertices
			srow >> tempst >> x >> y >> z;
			vertices.push_back(glmath::vec3(x*scale, y*scale, z*scale));
		} else if (row.substr(0,2) == "vt") {  // Texture Coordinates
			srow >> tempst >> x >> y;
			texcoords.push_back(glmath::vec2(x, y));
		} else if (row.substr(0,2) == "vn") {  // Normals
			srow >> tempst >> x >> y >> z;
			double sum = std::abs(x)+std::abs(y)+std::abs(z);
			if (sum == 0) throw std::runtime_error("Invalid normal in "+filepath.string()+":"+std::to_string(linenumber));
			x /= sum; y /= sum; z /= sum; // Normalize components
			normals.push_back(glmath::vec3(x, y, z));
		} else if (row.substr(0,2) == "f ") {  // Faces
			Face f;
			srow >> tempst; // Eat away prefix
			// Parse face point's coordinate references
			for (std::string fpoint; srow >> fpoint; ) {
				for (size_t i = 1; i <= 3; ++i) {
					std::string st_id(getWord(fpoint,i,'/'));
					if (!st_id.empty()) {
						// Vertex indices are 1-based in the file
						int v_id = std::stoi(st_id) - 1;
						switch (i) {
							case 1: f.vertices.push_back(v_id); break;
							case 2: f.texcoords.push_back(v_id); break;
							case 3: f.normals.push_back(v_id); break;
						}
					}
				}
			}
			if (!f.vertices.empty() && f.vertices.size() != 3)
				throw std::runtime_error("Only triangle faces allowed in "+filepath.string()+":"+std::to_string(linenumber));
			// Face must have equal number of v, vt, vn or none of a kind
			if (!f.vertices.empty()
			  && (f.texcoords.empty() || (f.texcoords.size() == f.vertices.size()))
			  && (f.normals.empty()   || (f.normals.size() == f.vertices.size()))) {
				m_faces.push_back(f);
			} else {
				throw std::runtime_error("Invalid face in "+filepath.string()+":"+std::to_string(linenumber));
			}
		}
	}
	// Construct a vertex array
	for (std::vector<Face>::const_iterator i = m_faces.begin(); i != m_faces.end(); ++i) {
		bool hasNormals = !i->normals.empty();
		bool hasTexCoords = !i->texcoords.empty();
		for (size_t j = 0; j < i->vertices.size(); ++j) {
			if (hasNormals) m_va.normal(normals[i->normals[j]]);
			if (hasTexCoords) m_va.texCoord(texcoords[i->texcoords[j]]);
			m_va.vertex(vertices[i->vertices[j]]);
		}
	}
}

void Object3d::load(fs::path const& filepath, fs::path const& texturepath, float scale) {
	if (!texturepath.empty()) m_texture = std::make_unique<Texture>(texturepath);
	loadWavefrontObj(filepath, scale);
}

void Object3d::draw() {
	UseShader us(getShader("3dobject"));
	if (m_texture) {
		UseTexture tex(*m_texture);
		m_va.draw(GL_TRIANGLES);
	} else {
		m_va.draw(GL_TRIANGLES);
	}
}

void Object3d::draw(float x, float y, float z, float s) {
	using namespace glmath;
	Transform trans(translate(vec3(x, y, z)) * scale(s));  // Move to position and scale
	draw();
}
