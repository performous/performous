#ifndef __PITCH_GRAPH_H__
#define __PITCH_GRAPH_H__

#include "../config.h"

class PitchGraph {
  public:
	PitchGraph(): clearPage(1), surface(), dc() {
		clear();
	}
	cairo_surface_t* renderPitch(double pitch, double time, double volume);
	cairo_surface_t* getCurrent() { return surface; }
	void clear();
	~PitchGraph();
  private:
	bool clearPage;
	cairo_surface_t* surface;
	cairo_t* dc;
};

#endif
