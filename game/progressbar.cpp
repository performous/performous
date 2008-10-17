#include "progressbar.hh"

#include <iostream>
#include <stdexcept>

void ProgressBar::draw(float value) {
	value = std::min(std::max(0.0f, value), 1.0f);
	value = m_begin + value * (m_end - m_begin);
	m_bg.draw(dimensions);
	float w = dimensions.w();
	float h = dimensions.h();
	switch (m_mode) {
	  case HORIZONTAL:
		{
			Dimensions d(dimensions.x1(), dimensions.y1(), value * dimensions.w(), dimensions.h());
			m_bar.draw(d, TexCoords(0.0, 0.0, value, 1.0));
		}
		return;
	  case VERTICAL:
		{
			float h = dimensions.h();
			Dimensions d(dimensions.x1(), dimensions.y1() + (1.0f - value) * h, dimensions.w(), value * h);
			m_bar.draw(d, TexCoords(0.0, 0.0, 1.0, value));
		}
		return;
	  case CIRCULAR:
		{
			UseTexture texblock(m_bar);
			throw std::logic_error("ProgressBar::draw(): CIRCULAR not implemented yet");  // TODO: Implement
		}
		return;
	}
	throw std::logic_error("ProgressBar::draw(): unknown m_mode value");
}
