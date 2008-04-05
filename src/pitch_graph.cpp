#include <pitch_graph.h>
#include <cmath>

cairo_surface_t* PitchGraph::renderPitch(double pitch, double time, double volume) {
	double lastPitch,lastTime;
	cairo_get_current_point(dc, &lastTime ,&lastPitch);
	if (pitch == 0.0);
	else if (clearPage) clearPage = 0;
	else if (lastTime < time) {
		cairo_new_path(dc);
		cairo_move_to(dc, lastTime, pitch);
		cairo_line_to(dc, time, pitch);
		cairo_set_line_width(dc, 0.02);
		cairo_set_source_rgba(this->dc, 52.0/255.0, 101.0/255.0, 164.0/255.0, 0.9);
		cairo_stroke_preserve(dc);
		double oldPitch = (std::abs(lastPitch - pitch) < 0.005 ? lastPitch : pitch);
		cairo_new_path(dc);
		cairo_move_to(dc, lastTime, oldPitch);
		cairo_line_to(dc, time, pitch);
		cairo_set_line_width(dc, 0.003 * volume);
		cairo_set_source_rgba(this->dc, 0.0, 0.0, 0.0, 1.0);
		cairo_stroke_preserve(dc);
	}
	cairo_move_to(dc, time, pitch);
	return surface;
}

void PitchGraph::clear() {
	if (dc) cairo_destroy(dc);
	if (surface) cairo_surface_destroy(surface);
	double width = 800.0, height = 600.0; // FIXME!
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	dc = cairo_create(surface);
	cairo_scale(dc, width, height);
	cairo_new_path(dc);
	clearPage = 1;
}

PitchGraph::~PitchGraph() {
	if (dc) cairo_destroy(dc);
	if (surface) cairo_surface_destroy(surface);
}
