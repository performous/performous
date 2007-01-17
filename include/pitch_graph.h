#ifndef __PITCH_GRAPH_H__
#define __PITCH_GRAPH_H__

#include "../config.h"

class PitchGraph
{
	public:
	PitchGraph(int width, int height);

	cairo_surface_t* renderPitch(double pitch, double time);
	cairo_surface_t* getCurrent() {return this->surface;}
	
	void clear();

	~PitchGraph();
	
	private:
	int width;
	int height;
	bool clearPage;

	cairo_surface_t* surface;
	cairo_t* dc;
};

#endif
