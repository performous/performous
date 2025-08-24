#include "graphiccontext.hh"
#include "text.hh"

#include "effect/effectmanager.hh"
#include <configuration.hh>

GraphicContext::GraphicContext(Window& window, EffectManager& manager)
: m_window(window), m_effectManager(manager) {
	//addFont("default", "mainmenu_option.svg");
	addFont("ui", "mainmenu_comment.svg");
}

ThemeUI const& GraphicContext::getTheme() const {
	return m_theme;
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

std::shared_ptr<SvgTxtTheme> GraphicContext::makeSvgText(std::string const&) {
	const auto font = m_fonts.begin()->second;

	return std::make_shared<SvgTxtTheme>(font.first, font.second);
}

void GraphicContext::add(EffectPtr effect) {
	m_effectManager.add(effect);
}

void GraphicContext::remove(EffectPtr effect) {
	m_effectManager.remove(effect);
}

Window& GraphicContext::getWindow() {
	return m_window;
}
