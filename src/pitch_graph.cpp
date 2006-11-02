#include <pitch_graph.h>
#include <cairo/cairo.h>

#include <iostream>
#include <math.h>

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
	double lastPitch,lastTime;
	cairo_get_current_point(dc,&lastTime , &lastPitch);

	if( pitch == 0.0 ){
		cairo_move_to(dc, time, pitch);
		cairo_stroke_preserve(dc);
		return surface;
	}

	if(clearPage)
	{
		cairo_move_to(dc, time, pitch);
		clearPage = 0;
	}
	else
	{
		if( lastPitch == 0 )
			cairo_move_to(dc, time, pitch);
		else if( fabs( lastPitch-pitch) < 0.001 )
			cairo_line_to(dc, time, pitch);
		else {
			cairo_line_to(dc, time, lastPitch);
			cairo_move_to(dc, time, pitch);
		}
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
	//cairo_set_line_cap(dc,CAIRO_LINE_CAP_ROUND);


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
