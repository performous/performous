#pragma once

#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>
#include <string>

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


class Object3d {
  private:
	std::vector<Vertex> m_vertices;
	std::vector<TexCoord> m_texcoords;
	std::vector<Vertex> m_normals;
	std::vector<Face> m_faces;
	//boost::scoped_ptr<Texture> m_texture;	
	/// load a Wavefront .obj 3d object file
	void loadWavefrontObj(std::string filepath, float scale = 1.0);
  public:
	/// constructors
	Object3d();
	Object3d(std::string filepath, float scale = 1.0) {
		//m_texture.reset(new Texture("button.svg"));
		loadWavefrontObj(filepath, scale);
	}
	/// draws the object
	void draw(float x = 0, float y = 0, float z = 0) const;
};
