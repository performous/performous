#pragma once

#include <memory>
#include <string>

#include "../opengl_text.hh"

class GraphicContext;

class Text {
public:
	Text(std::string const& text = {});

	void setText(std::string const& text);
	std::string getText() const;
	void bind(GraphicContext& gc);
	void draw(float x, float y);
	void draw(float x, float y, float width, float height);
	void drawCentered(float x, float y, float width, float height);

	//float measureWidth(std::string const&) const;
	//float measureHeight(std::string const&) const;

private:
	GraphicContext* m_gc = nullptr;
	std::string m_text;
	std::shared_ptr<SvgTxtTheme> m_svg;
};


