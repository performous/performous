#pragma once

#include "texture.hh"

/// simple progressbar class
class ProgressBar {
  public:
	/// type of progressbar
	enum class Mode { HORIZONTAL, VERTICAL, CIRCULAR };
	/**
	* Construct a new progress bar.
	* @param bg filename of background image
	* @param bar filename of the bar image
	* @param mode specifies bar appearance
	* @param begin margin before the bar begins [0, 1]
	* @param end margin after the bar ends [0, 1]
	* @param sliding makes the bar move; the texture is anchored at bar end rather than at bar beginning
	**/
	ProgressBar(fs::path const& bg, fs::path const& bar, Mode mode = Mode::HORIZONTAL, float begin = 0.0f, float end = 0.0f, bool sliding = false);
	/** Draw a progress bar with the given percentage [0, 1] **/
	void draw(Window& window, float value);
  private:
	Texture m_bg, m_bar;
	Mode m_mode;
	float m_begin, m_end;
	bool m_sliding;
  public:
	/// dimensions of progressbar
	Dimensions dimensions;
};

