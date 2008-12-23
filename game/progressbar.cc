#include "progressbar.hh"

#include "util.hh"
#include <iostream>
#include <stdexcept>

void ProgressBar::draw(float value) {
	value = clamp(value);
	float scale = 1.0 - m_begin - m_end;
	m_bg.draw(dimensions);
	float w = dimensions.w();
	float h = dimensions.h();
	switch (m_mode) {
	  case HORIZONTAL:
		{
			TexCoords tex;
			if (m_sliding) { tex.x2 = 1.0f - m_end; tex.x1 = tex.x2 - value * scale; }
			else { tex.x1 = m_begin; tex.x2 = tex.x1 + value * scale; }
			m_bar.draw(Dimensions(dimensions.x1() + m_begin * w, dimensions.y1(), value * scale * w, h), tex);
		}
		break;
	  case VERTICAL:
		{
			TexCoords tex;
			if (m_sliding) { tex.y1 = 1.0f - m_end; tex.y2 = tex.y1 + value * scale; }
			else { tex.y2 = 1.0f - m_begin; tex.y1 = tex.y2 - value * scale; }
			m_bar.draw(Dimensions(dimensions.x1(), 0.0f, w, value * scale * h).bottom(dimensions.y2() - m_begin * h), tex);
		}
		break;
	  case CIRCULAR:
		{
			UseTexture texblock(m_bar);
			throw std::logic_error("ProgressBar::draw(): CIRCULAR not implemented yet");  // TODO: Implement
		}
		break;
		default:
			throw std::logic_error("ProgressBar::draw(): unknown m_mode value");
	}
}
