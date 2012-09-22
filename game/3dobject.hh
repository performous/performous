#pragma once

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include "glshader.hh"

// TODO: Exception handling
// TODO: Texture loading

class Texture;

/// A class representing 3d object
/// Non-copyable because of display lists getting messed up
class Object3d: boost::noncopyable {
  private:
	glutil::VertexArray m_va;
	boost::scoped_ptr<Texture> m_texture; /// texture
	/// load a Wavefront .obj 3d object file
	void loadWavefrontObj(std::string filepath, float scale = 1.0);
  public:
	Object3d() {}
	/// constructors
	Object3d(std::string filepath, std::string texturepath = "", float scale = 1.0) {
		load(filepath, texturepath, scale);
	}
	/// load a new object file
	void load(std::string filepath, std::string texturepath = "", float scale = 1.0);
	void drawVBO();
	/// draws the object
	void draw(float x = 0, float y = 0, float z = 0, float s = 1.0);
};
