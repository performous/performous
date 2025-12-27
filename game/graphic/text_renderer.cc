#include "text_renderer.hh"

#include <pango/pangocairo.h>

#include <memory>

namespace {
	PangoAlignment parseAlignment(std::string const& fontalign) {
		if (fontalign == "start") return PANGO_ALIGN_LEFT;
		if (fontalign == "center" || fontalign == "middle") return PANGO_ALIGN_CENTER;
		if (fontalign == "end") return PANGO_ALIGN_RIGHT;
		throw std::logic_error("Unknown font alignment '" + fontalign + "'");
	}

	PangoWeight parseWeight(std::string const& fontweight) {
		if (fontweight == "normal") return PANGO_WEIGHT_NORMAL;
		if (fontweight == "bold") return PANGO_WEIGHT_BOLD;
		if (fontweight == "bolder") return PANGO_WEIGHT_ULTRABOLD;
		throw std::logic_error("Unknown font weight '" + fontweight + "'");
	}

	PangoStyle parseStyle(std::string const& fontstyle) {
		if (fontstyle == "normal") return PANGO_STYLE_NORMAL;
		if (fontstyle == "italic") return PANGO_STYLE_ITALIC;
		if (fontstyle == "oblique") return PANGO_STYLE_OBLIQUE;
		throw std::logic_error("Unknown font style '" + fontstyle + "'");
	}

	void alignFactor(float& factor) {
		factor *= 2.0f;  // HACK to improve text quality without affecting compatibility with old versions
	}
}

/**
  * \details   Given a block of text, which possibly contains newlines, and _some_ tab-separated columns,
  *            measure the width of each column in pixels.  Iterate over each line, determining the
  *            width of each column, and keep track of the maximum width seen for each column.  
  *
  * \note      Since typically we have a mix of single items lines and multi-item lines, we only 
  *            consider lines with tab-delimiters for measuring column widths.
  *
  * \param text    The text to measure
  * \param style   The style to use for measuring
  * \param m       The quality multiplier, typically doubles bitmap size
  * \param columns An array, into which we pack the maximum column widths
  *
  * \returns   The maximum number of columns over every line
  */
size_t TextRenderer::measureColumns(const std::string& text, const TextStyle& style, float m, std::array<float,MAX_COLUMNS>& columns)
{
	// Previous to this call, we've determined there is some columns in the text string.
	// Iterate over the text, measuring the pixel-width of each column in each line
	size_t maxCols = 0;
	std::istringstream issLines(text);
	std::string line;
	while (std::getline(issLines, line)) {
		// Only consider lines with tab-delimited columns
		size_t tabCount = static_cast<size_t>(std::count(line.begin(), line.end(), '\t'));
		if ( tabCount > 0 ) {
			std::istringstream issCols(line);
			std::string colText;
			size_t colIndex = 0;
			while (std::getline(issCols, colText, '\t')) {
				// Measure this column's text length in pixels
				auto size = measure(colText, style, m) * m;  // multiply by m, as bitmap size is m-times bigger
				columns[colIndex] = std::max(columns[colIndex], size.getWidth());
				++colIndex;
				if (colIndex >= columns.size() )
					break;  // exhausted array columns, give up on this line
			}
			maxCols = std::max(maxCols, colIndex);
		}
	}

	return maxCols;
}

OpenGLText TextRenderer::render(std::string const& text, TextStyle const& style, float m) {
	alignFactor(m);

	// Setup font settings
	auto alignment = parseAlignment(style.fontalign);
	std::shared_ptr<PangoFontDescription> desc(pango_font_description_new(), pango_font_description_free);
	pango_font_description_set_weight(desc.get(), parseWeight(style.fontweight));
	pango_font_description_set_style(desc.get(), parseStyle(style.fontstyle));
	pango_font_description_set_family(desc.get(), style.fontfamily.c_str());
	pango_font_description_set_absolute_size(desc.get(), style.fontsize * PANGO_SCALE * m);
	auto border = style.stroke_width * m;
	// Setup Pango context and layout
	std::shared_ptr<PangoContext> ctx(pango_font_map_create_context(pango_cairo_font_map_get_default()), g_object_unref);
	std::shared_ptr<PangoLayout> layout(pango_layout_new(ctx.get()), g_object_unref);
	pango_layout_set_alignment(layout.get(), alignment);
	pango_layout_set_font_description(layout.get(), desc.get());

	// IFF the text has columns (like the historical hiscores table) nicely format the columns
	std::array<float,MAX_COLUMNS> colWidths;
	bool hasColumns = (strchr(text.c_str(),'\t') != nullptr);
	if (hasColumns && measureColumns(text, style, m, colWidths) > 0) {
		// Pango defines tab-stops only for extra columns, so the first tab-stop is the pixel location of the second column. 
		// There is no concept of a first-column tab-stop (or justification)
		float position = 0.0f;
		const float SPACER = TextRenderer().measure("WWWW", style, m).getWidth();  // Can't be a constant, needs to match font & screen DPI, etc.
		int numTabs = static_cast<int>(colWidths.size());
		PangoTabArray *tabArray = pango_tab_array_new(numTabs, TRUE);  // positions in pixels
		for (int i=0; i<numTabs; i++) {
			position += colWidths[i] + SPACER;
			pango_tab_array_set_tab(tabArray, i, PANGO_TAB_LEFT, static_cast<int>(position+0.5f));
		}
		pango_layout_set_tabs(layout.get(), tabArray);
		pango_tab_array_free(tabArray);
	}

	pango_layout_set_text(layout.get(), text.c_str(), -1);  // Measuring changes the layout text, set it finally

	// Compute text extents
	auto width = 0.f;
	auto height = 0.f;
	{
		PangoRectangle rec;
		pango_layout_get_pixel_extents(layout.get(), nullptr, &rec);
		width = static_cast<float>(rec.width) + border;  // Add twice half a border for margins
		height = static_cast<float>(rec.height) + border;
	}


	// Create Cairo surface and drawing context
	std::shared_ptr<cairo_surface_t> surface(
	  cairo_image_surface_create(CAIRO_FORMAT_ARGB32, static_cast<int>(width), static_cast<int>(height)),
	  cairo_surface_destroy);
	std::shared_ptr<cairo_t> dc(cairo_create(surface.get()), cairo_destroy);
	// Keep things sharp and fast, we scale with OpenGL anyway...
	cairo_set_antialias(dc.get(), CAIRO_ANTIALIAS_FAST);
	cairo_push_group_with_content (dc.get(), CAIRO_CONTENT_COLOR_ALPHA);
	cairo_set_operator(dc.get(),CAIRO_OPERATOR_SOURCE);
	// Add Pango line and path to proper position on the DC
	cairo_move_to(dc.get(), 0.5f * border, 0.5f * border);  // Margins needed for border stroke to fit in
	pango_cairo_update_layout(dc.get(), layout.get());
	pango_cairo_layout_path(dc.get(), layout.get());
	// Render text
	if (style.fill_col.a > 0.0f) {
		cairo_set_source_rgba(dc.get(), style.fill_col.r, style.fill_col.g, style.fill_col.b, style.fill_col.a);
		cairo_fill_preserve(dc.get());
	}
	// Render text border
	if (style.stroke_col.a > 0.0f) {
		// Use proper line-joins and caps.
		cairo_set_line_join (dc.get(), style.LineJoin());
		cairo_set_line_cap (dc.get(), style.LineCap());
		cairo_set_line_join (dc.get(), style.LineJoin());
		cairo_set_miter_limit(dc.get(), style.stroke_miterlimit);
		cairo_set_line_width(dc.get(), border);
		cairo_set_source_rgba(dc.get(), style.stroke_col.r, style.stroke_col.g, style.stroke_col.b, style.stroke_col.a);
		cairo_stroke(dc.get());
	}
	cairo_pop_group_to_source(dc.get());
	cairo_set_operator(dc.get(),CAIRO_OPERATOR_OVER);
	cairo_paint (dc.get());
	// Load into m_texture (OpenGL texture)
	Bitmap bitmap(cairo_image_surface_get_data(surface.get()));
	bitmap.fmt = pix::Format::INT_ARGB;
	bitmap.linearPremul = true;
	auto bitmapWidth = static_cast<unsigned>(cairo_image_surface_get_width(surface.get()));
	auto bitmapHeight = static_cast<unsigned>(cairo_image_surface_get_height(surface.get()));
	bitmap.resize(bitmapWidth, bitmapHeight);
	auto texture = std::make_unique<Texture>();
	texture->load(bitmap, true);

	// We don't want text quality multiplier m to affect rendering size...
	return OpenGLText(text, texture, width / m, height / m);
}

Size TextRenderer::measure(const std::string& text, const TextStyle& style, float m) {
	alignFactor(m);

	// Setup font settings
	auto alignment = parseAlignment(style.fontalign);
	std::shared_ptr<PangoFontDescription> desc(pango_font_description_new(), pango_font_description_free);
	pango_font_description_set_weight(desc.get(), parseWeight(style.fontweight));
	pango_font_description_set_style(desc.get(), parseStyle(style.fontstyle));
	pango_font_description_set_family(desc.get(), style.fontfamily.c_str());
	pango_font_description_set_absolute_size(desc.get(), style.fontsize * PANGO_SCALE * m);
	auto border = style.stroke_width * m;
	// Setup Pango context and layout
	std::shared_ptr<PangoContext> ctx(pango_font_map_create_context(pango_cairo_font_map_get_default()), g_object_unref);
	std::shared_ptr<PangoLayout> layout(pango_layout_new(ctx.get()), g_object_unref);
	pango_layout_set_alignment(layout.get(), alignment);
	pango_layout_set_font_description(layout.get(), desc.get());
	pango_layout_set_text(layout.get(), text.c_str(), -1);

	// Compute text extents
	PangoRectangle rec;
	pango_layout_get_pixel_extents(layout.get(), nullptr, &rec);

	auto const width = static_cast<float>(rec.width) + border;  // Add twice half a border for margins
	auto const height = static_cast<float>(rec.height) + border;

	// We don't want text quality multiplier m to affect rendering size...
	return {width / m, height / m};
}

