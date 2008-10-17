#ifndef PERFORMOUS_PROGRESSBAR_HH
#define PERFORMOUS_PROGRESSBAR_HH

#include "surface.hh"

class ProgressBar {
  public:
	enum Mode { HORIZONTAL, VERTICAL, CIRCULAR };
	/**
	* Construct a new progress bar.
	* @param bg filename of background image
	* @param bar filename of the bar image
	* @param mode specifies bar appearance
	* @param begin how much of background to display when value is zero (left/bottom/begin margin)
	* @param end how much of background to display when value is one (right/top/end margin)
	* @param sliding makes the bar move; the texture is anchored at bar end rather than at bar beginning
	**/
	ProgressBar(std::string const& bg, std::string const& bar, Mode mode = HORIZONTAL, float begin = 0.0f, float end = 1.0f, bool sliding = false):
	  m_bg(bg), m_bar(bar), m_mode(mode), m_begin(begin), m_end(end), m_sliding(sliding), dimensions(m_bg.ar()) {}
	/** Draw a progress bar with the given percentage [0, 1] **/
	void draw(float value);
  private:
	Texture m_bg, m_bar;
	Mode m_mode;
	float m_begin, m_end;
	bool m_sliding;
  public:
	Dimensions dimensions;
};

#endif

