#pragma once

#include <string>
#include <vector>

class SingstarCover {
  public:
	SingstarCover(const std::string pak_file, unsigned int track_id);
	~SingstarCover();
	void write(const std::string filename);
  private:
	std::vector<char> m_image;
	unsigned int m_u, m_v , m_width, m_height;
};
