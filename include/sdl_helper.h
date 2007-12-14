#ifndef USNG_SDL_HELPER_H_INCLUDED
#define USNG_SDL_HELPER_H_INCLUDED

#include <boost/noncopyable.hpp>
#include <SDL/SDL_image.h>
#include <algorithm>

class SDLSurf: boost::noncopyable {
	SDL_Surface* m_surf;
  public:
	/** Takes ownership of an existing surface. **/
	explicit SDLSurf(SDL_Surface* surf): m_surf(surf) {}
	explicit SDLSurf(std::string const& filename): m_surf(IMG_Load(filename.c_str())) {
		if (filename.empty() || filename[filename.size() - 1] == '/') return; // Suppress useless warnings
		if (!m_surf) std::cout << "Unable to load " << filename << std::endl;
	}
	SDLSurf(std::string const& filename, double width, double height): m_surf() {
		SDLSurf surf(filename);
		if (!surf) return;
		// double factor = std::min(width / surf->w, height / surf->h);
		m_surf = zoomSurface(surf, width / surf->w, height / surf->h, 1);
	}
	~SDLSurf() { reset(); }
	operator SDL_Surface*() { return m_surf; }
	SDL_Surface* operator->() { return m_surf; }
	SDL_Surface& operator*() { return *m_surf; }
	void reset(SDL_Surface* surf = NULL) {
		SDL_FreeSurface(m_surf);
		m_surf = surf;
	}
};

#endif

