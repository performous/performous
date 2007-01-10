#ifndef __THEME_H__
#define __THEME_H__

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <cairosvg.h>
typedef struct _SRGBA {
        double r;
        double g;
        double b;
        double a;
} TRGBA;
typedef struct _SThemeTxt {
        double x;
        double y;
        cairo_text_extents_t extents;
        TRGBA fill_col;
        TRGBA stroke_col;
        double fontsize;
        double scale;
        char *text;

} TThemeTxt;
typedef struct _SThemeRect {
        double x;
        double y;
        double final_width;
        double final_height;
        double width;
        double height;
        TRGBA fill_col;
        TRGBA stroke_col;
} TThemeRect;
class CTheme {
        public:
        CTheme(int width, int height);
        ~CTheme();
        cairo_surface_t *PrintText(TThemeTxt *text); 
        cairo_surface_t *DrawRect(TThemeRect rect);
        cairo_text_extents_t GetTextExtents(TThemeTxt text);
        cairo_surface_t* getCurrent() {return this->surface;}
        void ParseSVGForText(char *filename, TThemeTxt *text);
        void ParseSVGForRect(char *filename, TThemeRect *rect);
        void clear();
        private: 
        int width;
        int height;
        cairo_surface_t *surface;
        cairo_t *dc;
        void walk_tree(xmlNode * a_node, TThemeTxt *text);
        void walk_tree(xmlNode * a_node, TThemeRect *rect);
        void getcolor(char *string, TRGBA *col);
};

class CThemeSing {
        public:
        CThemeSing();
        ~CThemeSing();
        CairoSVG *bg;
        CairoSVG *p1box;
        TThemeTxt timertxt; 
        TThemeTxt p1score;
        TThemeTxt lyricspast;
        TThemeTxt lyricsfuture;  
        TThemeTxt lyricshighlight;
        TThemeTxt lyricsnextsentence;
        TThemeRect progressfg;
        CTheme *theme;
        private:
};

#endif
