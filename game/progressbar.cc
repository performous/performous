#include "progressbar.hh"

#include "util.hh"

#include <stdexcept>

ProgressBar::ProgressBar(fs::path const& bg, fs::path const& bar, Mode mode, float begin, float end, bool sliding):
  m_bg(bg), m_bar(bar), m_mode(mode), m_begin(begin), m_end(end), m_sliding(sliding), dimensions(m_bg.dimensions)
{}

void ProgressBar::draw(float value) {
	value = clamp(value);
	float scale = 1.0f - m_begin - m_end;
	float off = (1.0f - value) * scale;  // Offset for sliding mode
	m_bg.draw(dimensions);
	switch (m_mode) {
	  case Mode::HORIZONTAL:
		{
			Dimensions dim = dimensions;
			TexCoords tex;
			if (m_sliding) { dim.move(-off * dim.w(), 0.0f); tex.x1 = m_begin + off; }
			else { tex.x1 = m_begin; tex.x2 = tex.x1 + value * scale; }
			m_bar.drawCropped(dim, tex);
		}
		break;
	  case Mode::VERTICAL:
		{
			Dimensions dim = dimensions;
			TexCoords tex;
			if (m_sliding) { dim.move(0.0f, off * dim.h()); tex.y2 = 1.0f - m_begin - off; }
			else { tex.y2 = 1.0f - m_begin; tex.y1 = tex.y2 - value * scale; }
			m_bar.drawCropped(dim, tex);
		}
		break;
	  case Mode::CIRCULAR:
		{
			UseTexture texblock(m_bar);
			throw std::logic_error("ProgressBar::draw(): CIRCULAR not implemented yet");  // TODO: Implement
		}
		break;
	  default: throw std::logic_error("ProgressBar::draw(): unknown m_mode value");
	}
	// Reset dimensions due to async image loading
	dimensions = m_bg.dimensions;
}
