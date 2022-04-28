#include "theme.hh"

#include "fs.hh"
#include "configuration.hh"

Theme::Theme()
{}
Theme::Theme(fs::path const& path) : bg(path)
{}

ThemeSongs::ThemeSongs():
	Theme(findFile("songs_bg.svg")),
	song(std::make_shared<SvgTxtTheme>(findFile("songs_song.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(4))),
	order(std::make_shared<SvgTxtTheme>(findFile("songs_order.svg"), config["graphic/text_lod"].f(), WrappingStyle().lyrics().setWidth(0.0f))),
	has_hiscore(std::make_shared<SvgTxtTheme>(findFile("songs_has_hiscore.svg"), config["graphic/text_lod"].f(), WrappingStyle().lyrics())),
	hiscores(std::make_shared<SvgTxtTheme>(findFile("songs_hiscores.svg"), config["graphic/text_lod"].f(), WrappingStyle().lyrics()))
{
	order->dimensions().screenBottom(-0.03f);
}

ThemePractice::ThemePractice():
	Theme(findFile("practice_bg.svg")),
	note(findFile("practice_note.svg")),
	sharp(findFile("practice_sharp.svg")),
	note_txt(std::make_shared<SvgTxtTheme>(findFile("practice_txt.svg"), config["graphic/text_lod"].f(), WrappingStyle().lyrics()))
{}

ThemeSing::ThemeSing():
	bg_top(findFile("sing_bg_top.svg")),
	bg_bottom(findFile("sing_bg_bottom.svg")),
	lyrics_now(std::make_shared<SvgTxtTheme>(findFile("sing_lyricscurrent.svg"), config["graphic/text_lod"].f(), WrappingStyle().lyrics())),
	lyrics_next(std::make_shared<SvgTxtTheme>(findFile("sing_lyricsnext.svg"), config["graphic/text_lod"].f(), WrappingStyle().lyrics())),
	timer(std::make_shared<SvgTxtTheme>(findFile("sing_timetxt.svg"), config["graphic/text_lod"].f(), WrappingStyle().lyrics())),
	songinfo(std::make_shared<SvgTxtTheme>(findFile("sing-songinfo.svg"), config["graphic/text_lod"].f(), WrappingStyle().lyrics()))
{
	lyrics_now->setHighlight(findFile("sing_lyricshighlight.svg"));
}

ThemeAudioDevices::ThemeAudioDevices():
	Theme(findFile("audiodevices_bg.svg")),
	device(std::make_shared<SvgTxtTheme>(findFile("mainmenu_comment.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(1))),
	device_bg(findFile("audiodevices_dev_bg.svg")),
	comment(std::make_shared<SvgTxtTheme>(findFile("mainmenu_comment.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(4))),
	comment_bg(findFile("mainmenu_comment_bg.svg"))
{}

ThemeIntro::ThemeIntro(unsigned short int showOpts):
	Theme(findFile("intro_bg.svg")),
	back_h(findFile("mainmenu_back_highlight.svg")),
	options(),
option_selected(std::make_shared<SvgTxtTheme>(findFile("mainmenu_option_selected.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(showOpts -1))),
	comment(std::make_shared<SvgTxtTheme>(findFile("mainmenu_comment.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(4))),
	short_comment(std::make_shared<SvgTxtTheme>(findFile("mainmenu_short_comment.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(1))),
	WebserverNotice(std::make_shared<SvgTxtTheme>(findFile("intro_webserver_notice.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText().ellipsizeNone(4))),
	comment_bg(findFile("mainmenu_comment_bg.svg")),
	short_comment_bg(findFile("mainmenu_scomment_bg.svg"))
{
std::clog << "themeIntro/debug: Constructed new theme." << std::endl;
}

ThemeInstrumentMenu::ThemeInstrumentMenu():
	Theme(findFile("instrumentmenu_bg.svg")),
	back_h(findFile("instrumentmenu_back_highlight.svg")),
	options(),
	option_selected(std::make_shared<SvgTxtTheme>(findFile("instrumentmenu_option_selected.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(1))),
	comment(std::make_shared<SvgTxtTheme>(findFile("instrumentmenu_comment.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(4)))
	//comment_bg(findFile("menu_comment_bg.svg"))
{
	comment->setAlign(Align::MIDDLE);
}
//at the moment just a copy of themeSong
ThemePlaylistScreen::ThemePlaylistScreen(unsigned short int showOpts):
	Theme(findFile("songs_bg.svg")),
	options(),
	option_selected(std::make_shared<SvgTxtTheme>(findFile("mainmenu_option_selected.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(showOpts -1))),
	comment(std::make_shared<SvgTxtTheme>(findFile("mainmenu_comment.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(4))),
	short_comment(std::make_shared<SvgTxtTheme>(findFile("mainmenu_short_comment.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(1))),
	comment_bg(findFile("mainmenu_comment_bg.svg")),
	short_comment_bg(findFile("mainmenu_scomment_bg.svg"))
{}

std::shared_ptr<SvgTxtTheme> ThemeInstrumentMenu::getCachedOption(const std::string& text) {
	if (options.find(text) != options.end()) return options.at(text);
	std::pair<std::string, std::shared_ptr<SvgTxtTheme>> kv = std::make_pair(text, std::make_shared<SvgTxtTheme>(findFile("instrumentmenu_option.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(1)));
	options.insert(std::move(kv));
	return options.at(text);
}
