#pragma once

#include <map>
#include <memory>
#include <string>

#include "../opengl_text.hh"
#include "../fs.hh"

class Text;

class GraphicContext {
public:
	GraphicContext();

	void addFont(std::string const& font, fs::path const& file);

	void draw(Text& text, float x, float y);
	void draw(Text& text, float x, float y, float width, float height);
	void drawCentered(Text& text, float x, float y, float width, float height);

	std::shared_ptr<SvgTxtTheme> makeSvgText(std::string const& text);

private:
	std::map<std::string, std::pair<fs::path, double>> m_fonts;
};
