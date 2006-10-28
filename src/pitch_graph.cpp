#include <pitch_graph.h>
#include <cairo/cairo.h>

#include <iostream>

PitchGraph::PitchGraph(int _width, int _height)
	:width(_width),
	height(_height),
	clearPage(1),
	surface(NULL),
	dc(NULL)
{
	clear();
}

cairo_surface_t* PitchGraph::renderPitch(double pitch, double time)
{
	if(clearPage)
	{
		cairo_move_to(dc, time, pitch);
		clearPage = 0;
	}
	else
	{
		cairo_line_to(dc, time, pitch);
	}

	cairo_stroke_preserve(dc);
	return surface;
}

void PitchGraph::clear()
{
	if(this->dc)
	{
		cairo_destroy(dc);
	}

	if(this->surface)
	{
		cairo_surface_destroy(surface);
	}
	
	surface = cairo_image_surface_create(
		CAIRO_FORMAT_ARGB32,
		width, height);
	dc = cairo_create(surface);
	cairo_scale(dc, width, height);
	cairo_set_line_width(dc, 0.01);


	cairo_new_path(dc);
	clearPage = 1;
	
	cairo_set_source_rgba(this->dc, 1, 0, 0, 1.0);
}


PitchGraph::~PitchGraph()
{
	if(this->dc)
	{
		cairo_destroy(dc);
	}

	if(this->surface)
	{
		cairo_surface_destroy(surface);
	}
}
