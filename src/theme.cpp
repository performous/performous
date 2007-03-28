#include <theme.h>
#include <screen.h>

CTheme::CTheme(int _width, int _height) 
        :width(_width),
        height(_height),
        surface(NULL),
        dc(NULL) {
        clear();
}
void CTheme::clear() {
        if(this->dc) {
            cairo_destroy(dc);
        }
        if(this->surface) {
            cairo_surface_destroy(surface);
        }
        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        dc = cairo_create(surface);
}
cairo_surface_t *CTheme::PrintText(TThemeTxt *text) {
        cairo_save(dc);
        cairo_scale(dc, width/text->svg_width, height/text->svg_height);
        cairo_select_font_face(dc, text->fontfamily, text->fontstyle, text->fontweight);
        cairo_set_font_size(dc, text->fontsize);
        cairo_move_to(dc, text->x, text->y);
        cairo_text_extents(dc, text->text, &(text->extents));
        cairo_move_to(dc, text->x - (text->extents.width - text->extents.width/text->scale)/2, text->y - (text->extents.height - text->extents.height/text->scale)/2);
        cairo_set_font_size(dc, text->fontsize * text->scale);
        cairo_text_path(dc, text->text);
        if (text->fill_col.r != -1 && text->fill_col.g != -1 && text->fill_col.b != -1) {
            cairo_set_source_rgba(dc, text->fill_col.r, text->fill_col.g, text->fill_col.b, text->fill_col.a);
            if (text->stroke_col.r != -1 && text->stroke_col.g != -1 && text->stroke_col.b != -1)
                cairo_fill_preserve(dc);
            else
                cairo_fill(dc);
        }
        if (text->stroke_col.r != -1 && text->stroke_col.g != -1 && text->stroke_col.b != -1) {
            cairo_set_line_width(dc, text->stroke_width);
            cairo_set_source_rgba(dc, text->stroke_col.r, text->stroke_col.b, text->stroke_col.g, text->stroke_col.a);
            cairo_stroke(dc);
        }
        cairo_restore(dc);
        return surface;
}
cairo_surface_t *CTheme::DrawRect(TThemeRect rect) {
        cairo_save(dc);
        cairo_scale(dc, width/rect.svg_width, height/rect.svg_height);
        cairo_rectangle(dc, rect.x, rect.y, rect.width, rect.height);
        if (rect.fill_col.r != -1 && rect.fill_col.g != -1 && rect.fill_col.b != -1) {
            cairo_set_source_rgba(dc, rect.fill_col.r, rect.fill_col.g, rect.fill_col.b, rect.fill_col.a);
            if (rect.stroke_col.r != -1 && rect.stroke_col.g != -1 && rect.stroke_col.b != -1)
                cairo_fill_preserve(dc);
            else
                cairo_fill(dc);
        }
        if (rect.stroke_col.r != -1 && rect.stroke_col.g != -1 && rect.stroke_col.b != -1) {
            cairo_set_line_width(dc, rect.stroke_width);
            cairo_set_source_rgba(dc, rect.stroke_col.r, rect.stroke_col.b, rect.stroke_col.g, rect.stroke_col.a);
            cairo_stroke(dc);
        }
        cairo_restore(dc);
        return surface;
}
cairo_text_extents_t CTheme::GetTextExtents(TThemeTxt text) {
        cairo_text_extents_t extents;
        cairo_select_font_face(dc, text.fontfamily, text.fontstyle, text.fontweight);
        cairo_set_font_size(dc, text.fontsize);
        cairo_move_to(dc, text.x, text.y);
        cairo_text_extents(dc, text.text, &extents);
        return extents;
}
void CTheme::ParseSVGForText(char *filename, TThemeTxt *text) {
        xmlDoc *doc = NULL;
        xmlNode *root_element = NULL;
        doc = xmlReadFile(filename, NULL, 0);
        root_element = xmlDocGetRootElement(doc);
        
        /* set some defaults */
        text->scale = 1.0;
        text->fontfamily[0]  ='\0';
        strncat(text->fontfamily,"Sans",4);
        text->fontstyle = CAIRO_FONT_SLANT_NORMAL;
        text->fontweight = CAIRO_FONT_WEIGHT_NORMAL;
        text->stroke_width = 0;

        walk_tree(root_element, text);
        xmlFreeDoc(doc);
        xmlCleanupParser();
}
void CTheme::ParseSVGForRect(char *filename, TThemeRect *rect) {
        xmlDoc *doc = NULL;
        xmlNode *root_element = NULL;
        doc = xmlReadFile(filename, NULL, 0);
        root_element = xmlDocGetRootElement(doc);
        /* set some defaults */
        rect->stroke_width = 0;
        
        walk_tree(root_element, rect);
        xmlFreeDoc(doc);
        xmlCleanupParser();
}
void CTheme::walk_tree(xmlNode * a_node, TThemeTxt *text)
{
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
                            sscanf((char *) xmlGetProp(cur_node, attr->name),"%lf",&(text->x));
                        } else if (!strcasecmp((char *) attr->name, "y")) {
                            sscanf((char *) xmlGetProp(cur_node, attr->name),"%lf",&(text->y));
                        } else if (!strcasecmp((char *) attr->name, "transform")) {
                            double x_trans, y_trans;
                            char tmp_string[10], *p; 
                            tmp_string[0] = '\0';
                            string = (char *) xmlGetProp(cur_node, attr->name);
                            if(!strncasecmp(string, "translate(", 10)) {
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
                        } else if (!strcasecmp((char *) attr->name, "style")) {
                           string = strtok((char *) xmlGetProp(cur_node, attr->name), ";");
                           while (string != NULL) {
                                if (!strncasecmp(string, "font-size:", 10)) {
                                    sscanf((string + 10),"%lf", &(text->fontsize));
                                } else if (!strncasecmp(string, "font-family:", 12)) {
                                    text->fontfamily[0]  ='\0';
                                    strncat((*text).fontfamily, (string + 12), 31);     /* copy no more than 31 bytes to not caue buffer overrun */
                                } else if (!strncasecmp(string, "font-style:", 11)) {
                                    if(!strncasecmp((string + 11), "normal", 6)) {
                                        text->fontstyle = CAIRO_FONT_SLANT_NORMAL;
                                    } else if(!strncasecmp((string + 11), "italic", 6)) {
                                        text->fontstyle = CAIRO_FONT_SLANT_ITALIC;
                                    } else if(!strncasecmp((string + 11), "oblique", 7)) {
                                        text->fontstyle = CAIRO_FONT_SLANT_OBLIQUE;
                                    }
                                } else if (!strncasecmp(string, "font-weight:", 12)) {
                                    if(!strncasecmp((string + 12), "normal", 6)) {
                                        text->fontweight = CAIRO_FONT_WEIGHT_NORMAL;
                                    } else if(!strncasecmp((string + 12), "bold", 4)) {
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

                        }

                    } else if (!strcasecmp((char *) cur_node->name, "svg")) {
                         if (!strcasecmp((char *) attr->name, "width")) {
                            sscanf((char *) xmlGetProp(cur_node, attr->name),"%lf",&(text->svg_width));
                        } else if (!strcasecmp((char *) attr->name, "height")) {
                            sscanf((char *) xmlGetProp(cur_node, attr->name),"%lf",&(text->svg_height));
                        } 
                    }
                    attr = attr->next;
                }
            }
        }
        walk_tree(cur_node->children, text);
    }
}
void CTheme::walk_tree(xmlNode * a_node, TThemeRect *rect)
{
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
                            sscanf((char *) xmlGetProp(cur_node, attr->name),"%lf",&(rect->x));
                        } else if (!strcasecmp((char *) attr->name, "y")) {
                            sscanf((char *) xmlGetProp(cur_node, attr->name),"%lf",&(rect->y));
                        } else if (!strcasecmp((char *) attr->name, "width")) {
                            sscanf((char *) xmlGetProp(cur_node, attr->name),"%lf",&(rect->final_width));
                            rect->width = rect->final_width;
                        } else if (!strcasecmp((char *) attr->name, "height")) {
                            sscanf((char *) xmlGetProp(cur_node, attr->name),"%lf",&(rect->final_height));
                            rect->height = rect->final_height;
                        } else if (!strcasecmp((char *) attr->name, "style")) {
                           string = strtok((char *) xmlGetProp(cur_node, attr->name), ";");
                           while (string != NULL) {
                                if (!strncasecmp(string, "fill:", 5)) {
                                    getcolor((string + 5), &(rect->fill_col));
                                } else if (!strncasecmp(string, "fill-opacity:", 13)) {
                                    sscanf((string + 13), "%lf", &(rect->fill_col.a));
                                } else if (!strncasecmp(string, "stroke:", 7)) {
                                    getcolor((string + 7), &(rect->stroke_col));
                                } else if (!strncasecmp(string, "stroke-opacity:", 15)) {
                                    sscanf((string + 15), "%lf", &(rect->stroke_col.a));
                                } else if (!strncasecmp(string, "stroke-width:", 13)) {
                                    sscanf((string + 13),"%lf", &(rect->stroke_width));
                                }
                                string = strtok(NULL, ";");
                           }

                        }

                    } else if (!strcasecmp((char *) cur_node->name, "svg")) {
                        if (!strcasecmp((char *) attr->name, "width")) {
                            sscanf((char *) xmlGetProp(cur_node, attr->name),"%lf",&(rect->svg_width));
                        } else if (!strcasecmp((char *) attr->name, "height")) {
                            sscanf((char *) xmlGetProp(cur_node, attr->name),"%lf",&(rect->svg_height));
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
            if(strlen(string) == 7) {
                sscanf((string+1), "%02x %02x %02x", &r, &g, &b);
                col->r = (double) r / 255;
                col->g = (double) g / 255;
                col->b = (double) b / 255;
            }
        } else if(!strcasecmp(string, "red")) {
            col->r = 1;
            col->g = col->b = 0;
        } else if(!strcasecmp(string, "lime")) {
            col->g = 1;
            col->r = col->b = 0;
        } else if(!strcasecmp(string, "blue")) {
            col->b = 1;
            col->r = col->g = 0;
        } else if(!strcasecmp(string, "black")) {
            col->r = col->g = col->b = 0;
        } else if(!strcasecmp(string, "silver")) {
            col->r = col->g = col->b = 0.75;
        } else if(!strcasecmp(string, "gray")) {
            col->r = col->g = col->b = 0.5;
        } else if(!strcasecmp(string, "white")) {
            col->r = col->g = col->b = 1;
        } else if(!strcasecmp(string, "maroon")) {
            col->r = 0.5;
            col->g = col->b = 0;
        } else if(!strcasecmp(string, "purple")) {
            col->g = 0.5;
            col->r = col->b = 0.5;
        } else if(!strcasecmp(string, "fuchsia")) {
            col->g = 0.5;
            col->r = col->b = 1;
        } else if(!strcasecmp(string, "green")) {
            col->g = 0.5;
            col->r = col->b = 0;
        } else if(!strcasecmp(string, "olive")) {
            col->b = 0;
            col->r = col->g = 0.5;
        } else if(!strcasecmp(string, "yellow")) {
            col->b = 0;
            col->r = col->g = 1;
        } else if(!strcasecmp(string, "navy")) {
            col->b = 0.5;
            col->r = col->g = 0;
        } else if(!strcasecmp(string, "teal")) {
            col->r = 0;
            col->g = col->b = 0.5;
        } else if(!strcasecmp(string, "aqua")) {
            col->r = 0;
            col->g = col->b = 1;
        } else if(!strcasecmp((string), "none")) {
            col->r = col->g = col->b = -1;
        }
    
}
CTheme::~CTheme() {
        if(this->dc) {
            cairo_destroy(dc);
        }
        if(this->surface) {
            cairo_surface_destroy(surface);
        }
}
CThemeSongs::CThemeSongs() {
	char * theme_path = new char[1024];
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	sm->getThemePathFile(theme_path,"songs_bg.svg");
	bg = new CairoSVG(theme_path, sm->getWidth(), sm->getHeight());

        theme = new CTheme(sm->getWidth(), sm->getHeight());
	sm->getThemePathFile(theme_path,"songs_song.svg");
	theme->ParseSVGForText(theme_path, &song);
	sm->getThemePathFile(theme_path,"songs_order.svg");
	theme->ParseSVGForText(theme_path, &order);
	
	delete[] theme_path;
}
CThemeSongs::~CThemeSongs() {
	delete bg;
	delete theme;
}
CThemeSing::CThemeSing() {
	char * theme_path = new char[1024];
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	sm->getThemePathFile(theme_path,"sing_bg.svg");
        bg = new CairoSVG(theme_path, sm->getWidth(), sm->getHeight());
	sm->getThemePathFile(theme_path,"sing_p1box.svg");
        p1box = new CairoSVG(theme_path, sm->getWidth(), sm->getHeight());
        theme = new CTheme(sm->getWidth(), sm->getHeight());
	sm->getThemePathFile(theme_path,"sing_timetxt.svg");
        theme->ParseSVGForText(theme_path, &timertxt);
	sm->getThemePathFile(theme_path,"sing_p1score.svg");
        theme->ParseSVGForText(theme_path, &p1score);
	sm->getThemePathFile(theme_path,"sing_lyricscurrent.svg");
        theme->ParseSVGForText(theme_path, &lyricspast);
	sm->getThemePathFile(theme_path,"sing_lyricscurrent.svg");
        theme->ParseSVGForText(theme_path, &lyricsfuture);
	sm->getThemePathFile(theme_path,"sing_lyricshighlight.svg");
        theme->ParseSVGForText(theme_path, &lyricshighlight);
	sm->getThemePathFile(theme_path,"sing_lyricsnext.svg");
        theme->ParseSVGForText(theme_path, &lyricsnextsentence);
	sm->getThemePathFile(theme_path,"sing_progressfg.svg");
        theme->ParseSVGForRect(theme_path, &progressfg);
	sm->getThemePathFile(theme_path,"sing_tostartfg.svg");
        theme->ParseSVGForRect(theme_path, &tostartfg);

	delete[] theme_path;
}
CThemeSing::~CThemeSing() {
        delete bg;
        delete p1box;
        delete theme;
}
