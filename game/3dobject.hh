#pragma once

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include "glshader.hh"

// TODO: Exception handling
// TODO: Texture loading

class Surface;

/// A class representing 3d object
/// Non-copyable because of display lists getting messed up
class Object3d: boost::noncopyable {
  private:
	glutil::VertexArray m_va;
	boost::scoped_ptr<Surface> m_texture; /// texture
	/// load a Wavefront .obj 3d object file
	void loadWavefrontObj(fs::path const& filepath, float scale = 1.0);
  public:
	Object3d() {}
	Object3d(fs::path const& filepath, fs::path const& texturepath = fs::path(), float scale = 1.0) {
		load(filepath, texturepath, scale);
	}
	/// load a new object file
	void load(fs::path const& filepath, fs::path const& texturepath = fs::path(), float scale = 1.0);
	/// draws the object (with texture if given)
	void draw();
	/// draws the object with a transform
	void draw(float x, float y, float z = 0, float s = 1.0);
};
