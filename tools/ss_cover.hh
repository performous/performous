#pragma once

#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>

namespace fs = boost::filesystem;

class SingstarCover {
  public:
	SingstarCover(const std::string pak_file, unsigned int track_id);
	~SingstarCover();
	void write(const fs::path filename);
  private:
	std::vector<char> m_image;
	unsigned int m_u, m_v , m_width, m_height;
};
