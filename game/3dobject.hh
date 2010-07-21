#pragma once

#include <vector>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include "surface.hh"
#include "glutil.hh"

// TODO: Exception handling
// TODO: Texture loading
// TODO: Switch to vertex arrays

/// Point in 3d space
struct Vertex {
	Vertex(float x = 0, float y = 0, float z = 0): x(x), y(y), z(z) {}
	float x;
	float y;
	float z;
};

/// 2d texture coordinate
struct TexCoord {
	TexCoord(float s = 0, float t = 0): s(s), t(t) {}
	float s;
	float t;
};

/// A polygon containing links to required point data
struct Face {
	std::vector<int> vertices;
	std::vector<int> texcoords;
	std::vector<int> normals;
};

/// A class representing 3d object
/// Non-copyable because of display lists getting messed up
class Object3d: boost::noncopyable {
  private:
	std::vector<Vertex> m_vertices; /// vertices
	std::vector<TexCoord> m_texcoords; /// texture coordinates
	std::vector<Vertex> m_normals; /// normals
	std::vector<Face> m_faces; /// faces
	GLuint m_displist; /// display list id
	boost::scoped_ptr<Texture> m_texture; /// texture
	/// load a Wavefront .obj 3d object file
	void loadWavefrontObj(std::string filepath, float scale = 1.0);
	/// generates a display list for the object
	void generateDisplayList();
  public:
	/// constructors
	Object3d(): m_displist(0) {};
	Object3d(std::string filepath, std::string texturepath = "", float scale = 1.0): m_displist(0) {
		load(filepath, texturepath, scale);
	}
	/// destructor
	~Object3d() {
		if (m_displist != 0) glDeleteLists(m_displist, 1);
	}
	/// load a new object file
	void load(std::string filepath, std::string texturepath = "", float scale = 1.0) {
		if (!texturepath.empty()) m_texture.reset(new Texture(texturepath));
		loadWavefrontObj(filepath, scale);
		generateDisplayList();
	}
	/// draws the object
	void draw(float x = 0, float y = 0, float z = 0, float s = 1.0) const {
		glutil::PushMatrix pm;
		glTranslatef(x, y, z); // Move to position
		if (s != 1.0) glScalef(s,s,s); // Scale if needed
		if (m_texture) {
			UseTexture tex(*m_texture);
			glCallList(m_displist);
		} else glCallList(m_displist);
	}
};
