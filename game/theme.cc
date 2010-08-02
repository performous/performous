#include "theme.hh"

#include "fs.hh"
#include "configuration.hh"

Theme::Theme()
{}
Theme::Theme(const std::string& path) : bg(path)
{}

ThemeSongs::ThemeSongs():
	Theme(getThemePath("songs_bg.svg")),
	song(getThemePath("songs_song.svg"), config["graphic/text_lod"].f()),
	order(getThemePath("songs_order.svg"), config["graphic/text_lod"].f()),
	has_hiscore(getThemePath("songs_has_hiscore.svg"), config["graphic/text_lod"].f()),
	hiscores(getThemePath("songs_hiscores.svg"), config["graphic/text_lod"].f())
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
	value(getThemePath("configuration_value.svg"), config["graphic/text_lod"].f()),
	comment(getThemePath("mainmenu_comment.svg"), config["graphic/text_lod"].f()),
	short_comment(getThemePath("mainmenu_short_comment.svg"), config["graphic/text_lod"].f()),
	comment_bg(getThemePath("mainmenu_comment_bg.svg")),
	short_comment_bg(getThemePath("mainmenu_scomment_bg.svg"))
{}

ThemeIntro::ThemeIntro():
	Theme(getThemePath("intro_bg.svg")),
	back_h(getThemePath("mainmenu_back_highlight.svg")),
	option(getThemePath("mainmenu_option.svg"), config["graphic/text_lod"].f()),
	option_selected(getThemePath("mainmenu_option_selected.svg"), config["graphic/text_lod"].f()),
	comment(getThemePath("mainmenu_comment.svg"), config["graphic/text_lod"].f()),
	comment_bg(getThemePath("mainmenu_comment_bg.svg"))
{
	back_h.dimensions.fixedHeight(0.08f);
}

ThemeInstrumentMenu::ThemeInstrumentMenu():
	Theme(getThemePath("instrumentmenu_bg.svg")),
	back_h(getThemePath("instrumentmenu_back_highlight.svg")),
	option(getThemePath("instrumentmenu_option.svg"), config["graphic/text_lod"].f()),
	option_selected(getThemePath("instrumentmenu_option_selected.svg"), config["graphic/text_lod"].f()),
	comment(getThemePath("instrumentmenu_comment.svg"), config["graphic/text_lod"].f())
	//comment_bg(getThemePath("menu_comment_bg.svg"))
{
	back_h.dimensions.fixedHeight(0.08f);
	comment.setAlign(SvgTxtTheme::CENTER);
}
