#include "image.hh"
#include "graphiccontext.hh"

Image::Image(std::string const& texture, Control* parent)
: Control(parent), m_texture(std::make_unique<Texture>(fs::path(texture))), m_background(findFile("mainmenu_back_highlight.svg")) {
}

Image::Image(Control* parent, std::string const& texture)
: Control(parent), m_texture(std::make_unique<Texture>(fs::path(texture))), m_background(findFile("mainmenu_back_highlight.svg")) {
}

void Image::setTexture(std::string const& texture) {
	m_texture = std::make_unique<Texture>(fs::path(texture));
}

void Image::draw(GraphicContext& gc) {
	m_background.dimensions.left(getX()).top(getY()).stretch(getWidth(), getHeight());
	m_background.draw();
	m_texture->dimensions.left(getX() + 0.01f).top(getY() + 0.01f).stretch(getWidth() - 0.02f, getHeight() - 0.02f);
	m_texture->draw();
}

