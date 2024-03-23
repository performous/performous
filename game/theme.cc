#include "theme.hh"

#include "fs.hh"
#include "configuration.hh"

Theme::Theme() {
}

Theme::Theme(fs::path const& path) : bg(path) {
}

ThemeSongs::ThemeSongs():
	Theme(findFile("songs_bg.svg")),
	song(findFile("songs_song.svg"), config["graphic/text_lod"].f()),
	order(findFile("songs_order.svg"), config["graphic/text_lod"].f()),
	has_hiscore(findFile("songs_has_hiscore.svg"), config["graphic/text_lod"].f()),
	hiscores(findFile("songs_hiscores.svg"), config["graphic/text_lod"].f())
{
	order.dimensions.screenBottom(-0.03f);
}

ThemePractice::ThemePractice():
	Theme(findFile("practice_bg.svg")),
	note(findFile("practice_note.svg")),
	sharp(findFile("practice_sharp.svg")),
	note_txt(findFile("practice_txt.svg"), config["graphic/text_lod"].f())
{}

ThemeGuitarTuner::ThemeGuitarTuner()
: Theme(findFile("bg_guitar_tuner.svg")), 
    fret(findFile("Guitar6Strings.png")), 
    bar(findFile("bar.svg")), 
	note_txt(findFile("practice_txt.svg"), config["graphic/text_lod"].f()) {
}

ThemeSing::ThemeSing():
	bg_top(findFile("sing_bg_top.svg")),
	bg_bottom(findFile("sing_bg_bottom.svg")),
	lyrics_now(findFile("sing_lyricscurrent.svg"), config["graphic/text_lod"].f()),
	lyrics_next(findFile("sing_lyricsnext.svg"), config["graphic/text_lod"].f()),
	timer(findFile("sing_timetxt.svg"), config["graphic/text_lod"].f()),
	songinfo(findFile("sing-songinfo.svg"), config["graphic/text_lod"].f())
{
	lyrics_now.setHighlight(findFile("sing_lyricshighlight.svg"));
}

ThemeAudioDevices::ThemeAudioDevices():
	Theme(findFile("audiodevices_bg.svg")),
	device(findFile("mainmenu_comment.svg"), config["graphic/text_lod"].f()),
	device_bg(findFile("audiodevices_dev_bg.svg")),
	comment(findFile("mainmenu_comment.svg"), config["graphic/text_lod"].f()),
	comment_bg(findFile("mainmenu_comment_bg.svg"))
{}

ThemeIntro::ThemeIntro():
	Theme(findFile("intro_bg.svg")),
	back_h(findFile("mainmenu_back_highlight.svg")),
	options(),
	option_selected(findFile("mainmenu_option_selected.svg"), config["graphic/text_lod"].f()),
	comment(findFile("mainmenu_comment.svg"), config["graphic/text_lod"].f()),
	short_comment(findFile("mainmenu_short_comment.svg"), config["graphic/text_lod"].f()),
	WebserverNotice(findFile("intro_webserver_notice.svg"), config["graphic/text_lod"].f()),
	comment_bg(findFile("mainmenu_comment_bg.svg")),
	short_comment_bg(findFile("mainmenu_scomment_bg.svg"))
{}

ThemeInstrumentMenu::ThemeInstrumentMenu():
	Theme(findFile("instrumentmenu_bg.svg")),
	back_h(findFile("instrumentmenu_back_highlight.svg")),
	options(),
	option_selected(findFile("instrumentmenu_option_selected.svg"), config["graphic/text_lod"].f()),
	comment(findFile("instrumentmenu_comment.svg"), config["graphic/text_lod"].f())
	//comment_bg(findFile("menu_comment_bg.svg"))
{
	comment.setAlign(SvgTxtTheme::Align::CENTER);
}
//at the moment just a copy of themeSong
ThemePlaylistScreen::ThemePlaylistScreen():
	Theme(findFile("songs_bg.svg")),
	options(),
	option_selected(findFile("mainmenu_option_selected.svg"), config["graphic/text_lod"].f()),
	comment(findFile("mainmenu_comment.svg"), config["graphic/text_lod"].f()),
	short_comment(findFile("mainmenu_short_comment.svg"), config["graphic/text_lod"].f()),
	comment_bg(findFile("mainmenu_comment_bg.svg")),
	short_comment_bg(findFile("mainmenu_scomment_bg.svg"))
{}

SvgTxtTheme& ThemeInstrumentMenu::getCachedOption(const std::string& text) {
	if (options.find(text) != options.end()) return (*options.at(text).get());
	std::pair<std::string, std::unique_ptr<SvgTxtTheme>> kv = std::make_pair(text, std::make_unique<SvgTxtTheme>(findFile("instrumentmenu_option.svg"), config["graphic/text_lod"].f()));
	options.insert(std::move(kv));
	return (*options.at(text).get());
}

