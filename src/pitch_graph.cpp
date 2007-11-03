#include <pitch_graph.h>

PitchGraph::PitchGraph(int _width, int _height):
  width(_width), height(_height), clearPage(1), surface(), dc()
{
	clear();
}

cairo_surface_t* PitchGraph::renderPitch(double pitch, double time, double volume) {
	double lastPitch,lastTime;
	cairo_get_current_point(dc, &lastTime ,&lastPitch);
	if (pitch == 0.0);
	else if (clearPage) clearPage = 0;
	else if (lastTime < time) {
		cairo_move_to(dc, lastTime, pitch);
		cairo_set_line_width(dc, 0.01 * volume);
		cairo_line_to(dc, time, pitch);
	}
	cairo_move_to(dc, time, pitch);
	cairo_stroke_preserve(dc);
	return surface;
}

void PitchGraph::clear() {
	if (dc) cairo_destroy(dc);
	if (surface) cairo_surface_destroy(surface);
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	dc = cairo_create(surface);
	cairo_scale(dc, width, height);
	cairo_new_path(dc);
	clearPage = 1;
	cairo_set_source_rgba(this->dc, 52.0/255.0, 101.0/255.0, 164.0/255.0, 1.0);
}


PitchGraph::~PitchGraph() {
	if (dc) cairo_destroy(dc);
	if (surface) cairo_surface_destroy(surface);
}
