#include "image.hh"
#include "graphiccontext.hh"
#include "game.hh"

Image::Image(std::string const& texture, Control* parent)
  : Control(parent) {
	setTexture(texture);
}

Image::Image(Control* parent, std::string const& texture)
  : Control(parent) {
	setTexture(texture);
}

void Image::setTexture(std::string const& texture) {
	if (!isInitialied()) {
		m_path = texture;
		return;
	}
	if (m_path == texture)
		return;

	m_path = texture;

	auto& game = getGame();

	std::cout << "path: " << m_path << "   " << fs::path(m_path).is_absolute() << std::endl;

	if (fs::path(m_path).is_absolute())
		m_texture = game.getTextureManager().get(m_path);
	else
		m_texture = game.getTextureManager().get(findFile(m_path));
}

void Image::draw(GraphicContext& gc) {
	auto const xOffset = getWidth() * 0.05f;
	auto const yOffset = getHeight() * 0.05f;
	auto const width = getWidth() * 0.9f;
	auto const height = getHeight() * 0.9f;

	drawFocus(gc);

	m_background.dimensions.left(getX()).top(getY()).stretch(getWidth(), getHeight());
	m_background.draw(gc.getWindow());

	m_texture.dimensions.left(getX() + xOffset).top(getY() + yOffset).stretch(width, height);
	m_texture.draw(gc.getWindow());
}

void Image::initialize(Game& game) {
	m_background = game.getTextureManager().get(findFile("mainmenu_back_highlight.svg"));

	Control::initialize(game);

	setTexture(m_path);
}

