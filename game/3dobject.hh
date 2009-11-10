#pragma once

#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/noncopyable.hpp>
#include "surface.hh"
#include "glutil.hh"

// TODO: Exception handling
// TODO: Texture loading

struct Vertex {
	Vertex(float x = 0, float y = 0, float z = 0): x(x), y(y), z(z) {}
	float x;
	float y;	
	float z;
};

struct TexCoord {
	TexCoord(float s = 0, float t = 0): s(s), t(t) {}
	float s;
	float t;
};

struct Face {
	std::vector<int> vertices;
	std::vector<int> texcoords;
	std::vector<int> normals;
};


class Object3d: boost::noncopyable {
  private:
	std::vector<Vertex> m_vertices;
	std::vector<TexCoord> m_texcoords;
	std::vector<Vertex> m_normals;
	std::vector<Face> m_faces;
	GLuint m_displist;
	//boost::scoped_ptr<Texture> m_texture;	
	/// load a Wavefront .obj 3d object file
	void loadWavefrontObj(std::string filepath, float scale = 1.0);
	/// generates a display list for the object
	void generateDisplayList();
  public:
	/// constructors
	Object3d(): m_displist(0) {};
	Object3d(std::string filepath, float scale = 1.0): m_displist(0) {
		load(filepath, scale);
	}
	/// destructor
	~Object3d() {
		if (m_displist != 0) glDeleteLists(m_displist, 1);
	}
	/// load a new object file
	void load(std::string filepath, float scale = 1.0) {
		//m_texture.reset(new Texture("button.svg"));
		loadWavefrontObj(filepath, scale);
		generateDisplayList();
	}
	/// draws the object
	void draw(float x = 0, float y = 0, float z = 0) const {
		glTranslatef(x, y, z);
		glCallList(m_displist);
		glTranslatef(-x, -y, -z);
	}
};
