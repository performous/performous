#pragma once

#include <map>
#include <memory>
#include <string>

#include "../opengl_text.hh"
#include "../fs.hh"

#include "ui/effect/effectmanager.hh"

class Text;

class GraphicContext {
public:
	GraphicContext(EffectManager&);

	void addFont(std::string const& font, fs::path const& file);

	void draw(Text& text, float x, float y);
	void draw(Text& text, float x, float y, float width, float height);
	void drawCentered(Text& text, float x, float y, float width, float height);

	std::shared_ptr<SvgTxtTheme> makeSvgText(std::string const& text);

	void add(EffectPtr);
	void remove(EffectPtr);

private:
	EffectManager& m_effectManager;
	std::map<std::string, std::pair<fs::path, double>> m_fonts;
};
