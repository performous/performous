#include "theme.hh"

#include "fs.hh"
#include "configuration.hh"

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
	note.dimensions.fixedHeight(0.03f);
	sharp.dimensions.fixedHeight(0.09f);
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

ThemeIntro::ThemeIntro():
	Theme(getThemePath("intro_bg.svg")),
	back_h(getThemePath("menu_back_highlight.svg")),
	option_selected(getThemePath("menu_option_selected.svg")),
	comment(getThemePath("menu_comment.svg"), config["graphic/text_lod"].f()),
	comment_bg(getThemePath("menu_comment_bg.svg"))
{
	back_h.dimensions.fixedHeight(0.08f);
	option.push_back(new SvgTxtTheme(getThemePath("menu_option.svg"), config["graphic/text_lod"].f()));
	option.push_back(new SvgTxtTheme(getThemePath("menu_option.svg"), config["graphic/text_lod"].f()));
	option.push_back(new SvgTxtTheme(getThemePath("menu_option.svg"), config["graphic/text_lod"].f()));
	option.push_back(new SvgTxtTheme(getThemePath("menu_option.svg"), config["graphic/text_lod"].f()));
}
