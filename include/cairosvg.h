#ifndef __CAIROSVG_H__
#define __CAIROSVG_H__

#include "../config.h"
#include <string>

/**
* SVG to Cairo context loading class. This class enables SVG loading into a cairo context with
*  both libsvg-cairo and librsvg. Rendering is way better with librsvg and libsvg-cairo is not
*  maintained anymore
 */
class CairoSVG {
  public:
	/**
	* Constructor
	* This constructor builds the cairo and the SDL surface according to an SVG file.
	* @param filename SVG filename
	* @param width destination surfaces width
	* @param height destination surfaces heigth
	 */
	CairoSVG(std::string const& filename, unsigned int _width, unsigned int _height);
	/**
	* Constructor
	* This constructor builds the cairo and the SDL surface according to an SVG stream (buffer).
	* @param data SVG buffer
	* @param data_len buffer length
	* @param width destination surfaces width
	* @param height destination surfaces heigth
	 */
  	CairoSVG(const char* data, size_t data_len ,unsigned int _width, unsigned int _height);
	/**
	* Destructor
	 */
 	~CairoSVG();
	/**
	* This method returns the SDL surface
	 */
	SDL_Surface* getSDLSurface() { return sdl_svg; };
	/**
	* This method returns the cairo surface
	 */
	cairo_surface_t* getCairoSurface() { return surface; };
  private:
	cairo_surface_t* surface;
	SDL_Surface* sdl_svg;
};

#endif
