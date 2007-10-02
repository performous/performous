#include <screen.h>
#include <stdexcept>

#ifndef THEMES_DIR
#define THEMES_DIR "/usr/local/share/ultrastar-ng/themes/"
#endif


template<> CScreenManager* CSingleton<CScreenManager>::ms_CSingleton = NULL;

CScreenManager::CScreenManager(unsigned int width, unsigned int height, std::string const& theme):
  m_finished(false),
  currentScreen(),
  screen(),
  audio(),
  songs(),
  videoDriver(),
  m_fullscreen(),
  m_width(width),
  m_height(height),
  m_theme(theme)
{}

CScreenManager::~CScreenManager()
{
	delete audio;
	delete songs;
}

void CScreenManager::activateScreen(std::string const& name) {
	CScreen* s = getScreen(name);
	if (currentScreen) currentScreen->exit();
	currentScreen = s;
	currentScreen->enter();
}

CScreen* CScreenManager::getScreen(std::string const& name) {
	screenmap_t::iterator it = screens.find(name);
	if (it != screens.end()) return it->second;
	throw std::invalid_argument("Screen " + name + " does not exist");
}

std::string CScreenManager::getThemePathFile(std::string const& file)
{
	if (m_theme.empty()) throw std::logic_error("CScreenManager::getThemePathFile(): m_theme is empty");
	return (*m_theme.rbegin() == '/' ? m_theme : THEMES_DIR + m_theme + "/") + file;
}

