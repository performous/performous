#include "theme.hh"

#include "fs.hh"
#include "configuration.hh"
#include "screen.hh"
#include <cairo.h>
#include <pango/pangocairo.h>

Theme::Theme()
{}
Theme::Theme(const std::string path) : bg(path)
{}

ThemeSongs::ThemeSongs():
	Theme(getThemePath("songs_bg.svg")),
	song(getThemePath("songs_song.svg"), config["graphic/text_lod"].f()),
	order(getThemePath("songs_order.svg"), config["graphic/text_lod"].f())
{}

ThemePractice::ThemePractice():
	Theme(getThemePath("practice_bg.svg")),
	note(getThemePath("practice_note.svg")),
	sharp(getThemePath("practice_sharp.svg")),
	note_txt(getThemePath("practice_txt.svg"), config["graphic/text_lod"].f())
{
	note.dimensions.fixedHeight(0.03);
	sharp.dimensions.fixedHeight(0.09);
}

ThemeSing::ThemeSing():
	bg_top(getThemePath("sing_bg_top.svg")),
	bg_bottom(getThemePath("sing_bg_bottom.svg")),
	lyrics_now(getThemePath("sing_lyricscurrent.svg"), config["graphic/text_lod"].f()),
	lyrics_next(getThemePath("sing_lyricsnext.svg"), config["graphic/text_lod"].f()),
	timer(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f())
{
	lyrics_now.setHighlight(getThemePath("sing_lyricshighlight.svg"));
}

ThemeConfiguration::ThemeConfiguration():
	Theme(getThemePath("configuration_bg.svg")),
	item(getThemePath("configuration_item.svg"), config["graphic/text_lod"].f()),
	value(getThemePath("configuration_value.svg"), config["graphic/text_lod"].f())
{}

