#include <pitch_graph.h>
#include <cairo/cairo.h>

PitchGraph::PitchGraph(int _width, int _height)
	:width(width),
	height(height),
	clearPage(1)
{
	// TODO Initializers
	width = _width;
	height = _height;

	surface = cairo_image_surface_create(
		CAIRO_FORMAT_ARGB32,
		width, height);
	dc = cairo_create(surface);
	cairo_scale(dc, width, height);
	cairo_set_source_rgba(dc, 1, 0, 0, 1.0);
	cairo_set_line_width(dc, 0.01);

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
	cairo_new_path(dc);
	cairo_paint_with_alpha(dc, 0);
	clearPage = 1;
}


PitchGraph::~PitchGraph()
{
	cairo_destroy(dc);
	cairo_surface_destroy (surface);
}
