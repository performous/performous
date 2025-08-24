#pragma once

#include <map>
#include <memory>
#include <string>

#include "../opengl_text.hh"
#include "../fs.hh"

#include "effect/ieffect.hh"

#include "theme.hh"

class Text;
class EffectManager;

class GraphicContext {
public:
	GraphicContext(Window&, EffectManager&);

	ThemeUI const& getTheme() const;

	void addFont(std::string const& font, fs::path const& file);

	void draw(Text& text, float x, float y);
	void draw(Text& text, float x, float y, float width, float height);
	void drawCentered(Text& text, float x, float y, float width, float height);

	std::shared_ptr<SvgTxtTheme> makeSvgText(std::string const& text);

	void add(EffectPtr);
	void remove(EffectPtr);

	Window& getWindow();

private:
	Window& m_window;
	EffectManager& m_effectManager;
	ThemeUI m_theme;
	std::map<std::string, std::pair<fs::path, double>> m_fonts;
};
