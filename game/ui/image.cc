#include "image.hh"
#include "graphiccontext.hh"

Image::Image(std::string const& texture, Control* parent)
: Control(parent), m_background(findFile("mainmenu_back_highlight.svg")) {
	setTexture(texture);
}

Image::Image(Control* parent, std::string const& texture)
: Control(parent), m_background(findFile("mainmenu_back_highlight.svg")) {
	setTexture(texture);
}

void Image::setTexture(std::string const& texture) {
	if(m_path != texture) {
		m_path = texture;

		std::cout << "path: " << texture << "   " << fs::path(texture).is_absolute() << std::endl;

		if(fs::path(texture).is_absolute())
			m_texture = std::make_unique<Texture>(texture);
		else
			m_texture = std::make_unique<Texture>(findFile(texture));
}

void Image::draw(GraphicContext& gc) {
	auto const xOffset = getWidth() * 0.05;
	auto const yOffset = getHeight() * 0.05;
	auto const width = getWidth() * 0.9;
	auto const height = getHeight() * 0.9;

	m_background.dimensions.left(getX()).top(getY()).stretch(getWidth(), getHeight());
	m_background.draw();

	if(m_texture) {
		m_texture->dimensions.left(getX() + xOffset).top(getY() + yOffset).stretch(width, height);
		m_texture->draw();
	}
}

