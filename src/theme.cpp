#include "theme.hh"
#include "screen.hh"
#include <cairo.h>
#include <pango/pangocairo.h>

void CTheme::clear() {
	if (dc) cairo_destroy(dc);
	if (surface) cairo_surface_destroy(surface);
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1024, 1024);
	dc = cairo_create(surface);
}

cairo_surface_t *CTheme::PrintText(TThemeTxt *text) {
	PangoFontDescription *desc = pango_font_description_new();
	PangoLayout *layout = pango_cairo_create_layout(dc);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER );
	PangoRectangle rec;

	cairo_save(dc);

	pango_font_description_set_family(desc, text->fontfamily.c_str());
	pango_font_description_set_style (desc,PANGO_STYLE_NORMAL);
	pango_font_description_set_absolute_size (desc,text->fontsize * PANGO_SCALE);
	pango_layout_set_font_description (layout, desc);
	pango_layout_set_text (layout, text->text.c_str(), -1);
	pango_layout_get_pixel_extents (layout,NULL,&rec);
	text->extents.width = rec.width;
	text->extents.height = rec.height;
	text->extents.x_advance = rec.width+rec.x;
	text->extents.y_advance = rec.height+rec.y;
	pango_layout_set_font_description (layout, desc);
	pango_cairo_update_layout (dc, layout);
	cairo_move_to(dc,text->x - (rec.width-rec.width/text->scale)/2,text->y-text->fontsize * text->scale);
	cairo_scale(dc, text->scale, text->scale);
	pango_cairo_show_layout (dc, layout);
	pango_cairo_layout_path(dc,layout);
	if (text->fill_col.r != -1 && text->fill_col.g != -1 && text->fill_col.b != -1) {
		cairo_set_source_rgba(dc, text->fill_col.r, text->fill_col.g, text->fill_col.b, text->fill_col.a);
		if (text->stroke_col.r != -1 && text->stroke_col.g != -1 && text->stroke_col.b != -1) cairo_fill_preserve(dc);
		else cairo_fill(dc);
	}
	if (text->stroke_col.r != -1 && text->stroke_col.g != -1 && text->stroke_col.b != -1) {
		cairo_set_line_width(dc, text->stroke_width);
		cairo_set_source_rgba(dc, text->stroke_col.r, text->stroke_col.g, text->stroke_col.b, text->stroke_col.a);
		cairo_stroke(dc);
	}
	cairo_restore(dc);

	g_object_unref(layout);
	pango_font_description_free(desc);
	return surface;
}

cairo_text_extents_t CTheme::GetTextExtents(TThemeTxt text) {
	cairo_text_extents_t extents;

	PangoContext* ctx=NULL;
	ctx = pango_cairo_font_map_create_context ((PangoCairoFontMap*)pango_cairo_font_map_get_default());
	PangoFontDescription *desc = pango_font_description_new();
	PangoLayout *layout = pango_layout_new(ctx);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER );
	PangoRectangle rec;

	pango_font_description_set_family(desc, text.fontfamily.c_str());
	pango_font_description_set_style (desc,PANGO_STYLE_NORMAL);
	pango_font_description_set_absolute_size (desc,text.fontsize * PANGO_SCALE);
	pango_layout_set_font_description (layout, desc);
	pango_layout_set_text (layout, text.text.c_str(), -1);
	pango_layout_get_pixel_extents (layout,NULL,&rec);
	extents.width = rec.width;
	extents.height = rec.height;
	extents.x_advance = rec.width+rec.x;
	extents.y_advance = rec.height+rec.y;

	g_object_unref (layout);
	pango_font_description_free (desc);
	g_object_unref (ctx);

	return extents;
}

void CTheme::ParseSVGForText(std::string const& filename, TThemeTxt *text) {
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	doc = xmlReadFile(filename.c_str(), NULL, 0);
	root_element = xmlDocGetRootElement(doc);

	/* set some defaults */
	text->scale = 1.0;
	text->fontfamily = "Sans";
	text->fontstyle = CAIRO_FONT_SLANT_NORMAL;
	text->fontweight = CAIRO_FONT_WEIGHT_NORMAL;
	text->stroke_width = 0;

	walk_tree(root_element, text);
	xmlFreeDoc(doc);
	xmlCleanupParser();
}

void CTheme::ParseSVGForRect(std::string const& filename, TThemeRect *rect) {
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	doc = xmlReadFile(filename.c_str(), NULL, 0);
	root_element = xmlDocGetRootElement(doc);
	/* set some defaults */
	rect->stroke_width = 0;

	walk_tree(root_element, rect);
	xmlFreeDoc(doc);
	xmlCleanupParser();
}

void CTheme::walk_tree(xmlNode * a_node, TThemeTxt *text) {
	xmlNode *cur_node = NULL;
	xmlAttr *attr = NULL;
	char *string;
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
	   if (cur_node->type == XML_ELEMENT_NODE) { 
			if (cur_node->properties != NULL) {
				attr = cur_node->properties;
				while (attr != NULL) {
				   if (!strcasecmp((char *) cur_node->name, "text")) {
						if (!strcasecmp((char *) attr->name, "x")) {
							string = (char *) xmlGetProp(cur_node, attr->name);
							sscanf(string,"%lf",&(text->x));
							xmlFree(string);
						} else if (!strcasecmp((char *) attr->name, "y")) {
							string = (char *) xmlGetProp(cur_node, attr->name);
							sscanf(string,"%lf",&(text->y));
							xmlFree(string);
						} else if (!strcasecmp((char *) attr->name, "transform")) {
							double x_trans, y_trans;
							char tmp_string[10], *p; 
							tmp_string[0] = '\0';
							string = (char *) xmlGetProp(cur_node, attr->name);
							if (!strncasecmp(string, "translate(", 10)) {
								string += 10;
								p = strchr(string, ',');
								strncat(tmp_string, string, p - string);
								sscanf(tmp_string, "%lf", &x_trans);
								string = p + 1;
								p = strchr(string, ')');
								tmp_string[0] = '\0';
								strncat(tmp_string, string, p - string);
								sscanf(tmp_string, "%lf", &y_trans);
								text->x += x_trans;
								text->y += y_trans;
							}
							xmlFree(string);
						} else if (!strcasecmp((char *) attr->name, "style")) {
						   char * orig = (char *) xmlGetProp(cur_node, attr->name);
						   string = strtok(orig, ";");
						   while (string != NULL) {
								if (!strncasecmp(string, "font-size:", 10)) {
									sscanf((string + 10),"%lf", &(text->fontsize));
								} else if (!strncasecmp(string, "font-family:", 12)) {
									text->fontfamily = string + 12;
								} else if (!strncasecmp(string, "font-style:", 11)) {
									if (!strncasecmp((string + 11), "normal", 6)) {
										text->fontstyle = CAIRO_FONT_SLANT_NORMAL;
									} else if (!strncasecmp((string + 11), "italic", 6)) {
										text->fontstyle = CAIRO_FONT_SLANT_ITALIC;
									} else if (!strncasecmp((string + 11), "oblique", 7)) {
										text->fontstyle = CAIRO_FONT_SLANT_OBLIQUE;
									}
								} else if (!strncasecmp(string, "font-weight:", 12)) {
									if (!strncasecmp((string + 12), "normal", 6)) {
										text->fontweight = CAIRO_FONT_WEIGHT_NORMAL;
									} else if (!strncasecmp((string + 12), "bold", 4)) {
										text->fontweight = CAIRO_FONT_WEIGHT_BOLD;
									}
								} else if (!strncasecmp(string, "fill:", 5)) {
									getcolor((string + 5), &(text->fill_col));
								} else if (!strncasecmp(string, "fill-opacity:", 13)) {
									sscanf((string + 13), "%lf", &(text->fill_col.a));
								} else if (!strncasecmp(string, "stroke:", 7)) {
									getcolor((string + 7), &(text->stroke_col));
								} else if (!strncasecmp(string, "stroke-opacity:", 15)) {
									sscanf((string + 15), "%lf", &(text->stroke_col.a));
								} else if (!strncasecmp(string, "stroke-width:", 13)) {
									sscanf((string + 13),"%lf", &(text->stroke_width));
								}
								string = strtok(NULL, ";");
						   }
						   xmlFree(orig);

						}

					} else if (!strcasecmp((char *) cur_node->name, "svg")) {
						if (!strcasecmp((char *) attr->name, "width")) {
							string = (char *) xmlGetProp(cur_node, attr->name);
							sscanf(string,"%lf",&(text->svg_width));
							xmlFree(string);
						} else if (!strcasecmp((char *) attr->name, "height")) {
							string = (char *) xmlGetProp(cur_node, attr->name);
							sscanf(string,"%lf",&(text->svg_height));
							xmlFree(string);
						} 
					}
					attr = attr->next;
				}
			}
		}
		walk_tree(cur_node->children, text);
	}
}

void CTheme::walk_tree(xmlNode * a_node, TThemeRect *rect) {
	xmlNode *cur_node = NULL;
	xmlAttr *attr = NULL;
	char *string;
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
	   if (cur_node->type == XML_ELEMENT_NODE) { 
			if (cur_node->properties != NULL) {
				attr = cur_node->properties;
				while (attr != NULL) {
					if (!strcasecmp((char *) cur_node->name, "rect")) {
						if (!strcasecmp((char *) attr->name, "x")) {
							string = (char *) xmlGetProp(cur_node, attr->name);
							sscanf(string,"%lf",&(rect->x));
							xmlFree(string);
						} else if (!strcasecmp((char *) attr->name, "y")) {
							string = (char *) xmlGetProp(cur_node, attr->name);
							sscanf(string,"%lf",&(rect->y));
							xmlFree(string);
						} else if (!strcasecmp((char *) attr->name, "width")) {
							string = (char *) xmlGetProp(cur_node, attr->name);
							sscanf(string,"%lf",&(rect->final_width));
							xmlFree(string);
							rect->width = rect->final_width;
						} else if (!strcasecmp((char *) attr->name, "height")) {
							string = (char *) xmlGetProp(cur_node, attr->name);
							sscanf(string,"%lf",&(rect->final_height));
							xmlFree(string);
							rect->height = rect->final_height;
						} else if (!strcasecmp((char *) attr->name, "style")) {
						   string = (char *) xmlGetProp(cur_node, attr->name);
						   char * string_tmp = strtok(string, ";");
						   while (string_tmp != NULL) {
								if (!strncasecmp(string_tmp, "fill:", 5)) {
									getcolor((string_tmp + 5), &(rect->fill_col));
								} else if (!strncasecmp(string_tmp, "fill-opacity:", 13)) {
									sscanf((string_tmp + 13), "%lf", &(rect->fill_col.a));
								} else if (!strncasecmp(string_tmp, "stroke:", 7)) {
									getcolor((string_tmp + 7), &(rect->stroke_col));
								} else if (!strncasecmp(string_tmp, "stroke-opacity:", 15)) {
									sscanf((string_tmp + 15), "%lf", &(rect->stroke_col.a));
								} else if (!strncasecmp(string_tmp, "stroke-width:", 13)) {
									sscanf((string_tmp + 13),"%lf", &(rect->stroke_width));
								}
								string_tmp = strtok(NULL, ";");
						   }
						   xmlFree(string);

						}

					} else if (!strcasecmp((char *) cur_node->name, "svg")) {
						if (!strcasecmp((char *) attr->name, "width")) {
							string = (char *) xmlGetProp(cur_node, attr->name);
							sscanf(string,"%lf",&(rect->svg_width));
							xmlFree(string);
						} else if (!strcasecmp((char *) attr->name, "height")) {
							string = (char *) xmlGetProp(cur_node, attr->name);
							sscanf(string,"%lf",&(rect->svg_height));
							xmlFree(string);
						} 
					}
					attr = attr->next;
				}
			}
		}
		walk_tree(cur_node->children, rect);
	}
}

void CTheme::getcolor(char *string, TRGBA *col) {
		unsigned int r,g,b;
		if (string[0] == '#') {
			if (strlen(string) == 7) {
				sscanf((string+1), "%02x %02x %02x", &r, &g, &b);
				col->r = (double) r / 255;
				col->g = (double) g / 255;
				col->b = (double) b / 255;
			}
		} else if (!strcasecmp(string, "red")) {
			col->r = 1;
			col->g = col->b = 0;
		} else if (!strcasecmp(string, "lime")) {
			col->g = 1;
			col->r = col->b = 0;
		} else if (!strcasecmp(string, "blue")) {
			col->b = 1;
			col->r = col->g = 0;
		} else if (!strcasecmp(string, "black")) {
			col->r = col->g = col->b = 0;
		} else if (!strcasecmp(string, "silver")) {
			col->r = col->g = col->b = 0.75;
		} else if (!strcasecmp(string, "gray")) {
			col->r = col->g = col->b = 0.5;
		} else if (!strcasecmp(string, "white")) {
			col->r = col->g = col->b = 1;
		} else if (!strcasecmp(string, "maroon")) {
			col->r = 0.5;
			col->g = col->b = 0;
		} else if (!strcasecmp(string, "purple")) {
			col->g = 0.5;
			col->r = col->b = 0.5;
		} else if (!strcasecmp(string, "fuchsia")) {
			col->g = 0.5;
			col->r = col->b = 1;
		} else if (!strcasecmp(string, "green")) {
			col->g = 0.5;
			col->r = col->b = 0;
		} else if (!strcasecmp(string, "olive")) {
			col->b = 0;
			col->r = col->g = 0.5;
		} else if (!strcasecmp(string, "yellow")) {
			col->b = 0;
			col->r = col->g = 1;
		} else if (!strcasecmp(string, "navy")) {
			col->b = 0.5;
			col->r = col->g = 0;
		} else if (!strcasecmp(string, "teal")) {
			col->r = 0;
			col->g = col->b = 0.5;
		} else if (!strcasecmp(string, "aqua")) {
			col->r = 0;
			col->g = col->b = 1;
		} else if (!strcasecmp((string), "none")) {
			col->r = col->g = col->b = -1;
		}
	
}

CTheme::~CTheme() {
	if (dc) cairo_destroy(dc);
	if (surface) cairo_surface_destroy(surface);
}

CThemeSongs::CThemeSongs() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("songs_bg.svg")));
	theme.reset(new CTheme());
	theme->ParseSVGForText(sm->getThemePathFile("songs_song.svg"), &song);
	theme->ParseSVGForText(sm->getThemePathFile("songs_order.svg"), &order);
}

CThemePractice::CThemePractice() {
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("practice_bg.svg")));
	theme.reset(new CTheme());
	theme->ParseSVGForText(sm->getThemePathFile("practice_txt.svg"), &notetxt);
	theme->ParseSVGForRect(sm->getThemePathFile("practice_peak.svg"), &peak);
}

CThemeSing::CThemeSing() {
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("sing_bg.svg")));
	p1box.reset(new Surface(sm->getThemePathFile("sing_p1box.svg")));
	p2box.reset(new Surface(sm->getThemePathFile("sing_p2box.svg")));
	theme.reset(new CTheme());
	theme->ParseSVGForText(sm->getThemePathFile("sing_timetxt.svg"), &timertxt);
	theme->ParseSVGForText(sm->getThemePathFile("sing_p1score.svg"), &p1score);
	theme->ParseSVGForText(sm->getThemePathFile("sing_p2score.svg"), &p2score);
	theme->ParseSVGForText(sm->getThemePathFile("sing_lyricscurrent.svg"), &lyricspast);
	theme->ParseSVGForText(sm->getThemePathFile("sing_lyricscurrent.svg"), &lyricsfuture);
	theme->ParseSVGForText(sm->getThemePathFile("sing_lyricshighlight.svg"), &lyricshighlight);
	theme->ParseSVGForText(sm->getThemePathFile("sing_lyricsnext.svg"), &lyricsnextsentence);
	theme->ParseSVGForRect(sm->getThemePathFile("sing_progressfg.svg"), &progressfg);
	theme->ParseSVGForRect(sm->getThemePathFile("sing_tostartfg.svg"), &tostartfg);
}

CThemeScore::CThemeScore() {
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("score_bg.svg")));
	theme.reset(new CTheme());
	theme->ParseSVGForText(sm->getThemePathFile("score_txt.svg"), &normal_score);
	theme->ParseSVGForText(sm->getThemePathFile("score_rank.svg"), &rank);
	theme->ParseSVGForRect(sm->getThemePathFile("score_level.svg"), &level);
}

CThemeConfiguration::CThemeConfiguration() {
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	bg.reset(new Surface(sm->getThemePathFile("configuration_bg.svg")));
	theme.reset(new CTheme());
	theme->ParseSVGForText(sm->getThemePathFile("configuration_item.svg"), &item);
	theme->ParseSVGForText(sm->getThemePathFile("configuration_value.svg"), &value);
}

