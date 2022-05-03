#include "graphiccontext.hh"
#include "text.hh"

#include <configuration.hh>

GraphicContext::GraphicContext() {
	//addFont("default", "mainmenu_option.svg");
	addFont("ui", "mainmenu_comment.svg");
}

void GraphicContext::addFont(std::string const& font, fs::path const& file) {
	m_fonts.emplace(font, std::make_pair(findFile(file), config["graphic/text_lod"].f()));
}

void GraphicContext::draw(Text& text, float x, float y) {
	text.bind(*this);
	text.draw(x, y);
}

void GraphicContext::draw(Text& text, float x, float y, float width, float height) {
	text.bind(*this);
	text.draw(x, y, width, height);
}

void GraphicContext::drawCentered(Text& text, float x, float y, float width, float height) {
	text.bind(*this);
	text.drawCentered(x, y, width, height);
}

std::shared_ptr<SvgTxtTheme> GraphicContext::makeSvgText(std::string const& text) {
	const auto font = m_fonts.begin()->second;

	return std::make_shared<SvgTxtTheme>(font.first, font.second);
}
