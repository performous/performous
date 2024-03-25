#include "theme.hh"

#include "fs.hh"
#include "configuration.hh"

Theme::Theme()
{}

Theme::Theme(fs::path const& path) : bg(path)
{}

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
	if (options.find(text) != options.end()) 
		return (*options.at(text).get());
	auto kv = std::make_pair(text, std::make_unique<SvgTxtTheme>(findFile("instrumentmenu_option.svg"), config["graphic/text_lod"].f()));
	options.insert(std::move(kv));
	return (*options.at(text).get());
}

ThemeSongFilterScreen::ThemeSongFilterScreen()
: Theme(findFile("songfilter_bg.svg")) {
}

ThemeUI::ThemeUI()
: button_bg(findFile("button_bg.svg").string()),
	checkbox_checked(findFile("checkbox_checked.svg").string()),
	checkbox_unchecked(findFile("checkbox_unchecked.svg").string()),
	image_bg(findFile("image_bg.svg").string()),
	list_bg(findFile("list_bg.svg").string()),
	list_selected_bg(findFile("list_selected_bg.svg").string()),
	select_bg(findFile("select_bg.svg").string()),
	select_up_down(findFile("select_up_down.svg").string()),
	textbox_bg(findFile("textbox_bg.svg").string()),
	textbox_cursor(findFile("textbox_cursor.svg").string()),
	focus(findFile("ui_focused.svg").string())
{
}

std::unique_ptr<Texture> ThemeUI::getButtonBG() const {
	return std::make_unique<Texture>(button_bg);
}

std::unique_ptr<Texture> ThemeUI::getCheckboxCheck() const {
	return std::make_unique<Texture>(checkbox_checked);
}

std::unique_ptr<Texture> ThemeUI::getCheckboxUncheck() const {
	return std::make_unique<Texture>(checkbox_unchecked);
}

std::unique_ptr<Texture> ThemeUI::getImageBG() const {
	return std::make_unique<Texture>(image_bg);
}

std::unique_ptr<Texture> ThemeUI::getListBG() const {
	return std::make_unique<Texture>(list_bg);
}

std::unique_ptr<Texture> ThemeUI::getListSelectedBG() const {
	return std::make_unique<Texture>(list_selected_bg);
}

std::unique_ptr<Texture> ThemeUI::getSelectBG() const {
	return std::make_unique<Texture>(select_bg);
}

std::unique_ptr<Texture> ThemeUI::getSelectUpDown() const {
	return std::make_unique<Texture>(select_up_down);
}

std::unique_ptr<Texture> ThemeUI::getTextboxBG() const {
	return std::make_unique<Texture>(textbox_bg);
}

std::unique_ptr<Texture> ThemeUI::getTextboxCursor() const {
	return std::make_unique<Texture>(textbox_cursor);
}

std::unique_ptr<BorderDefinition> ThemeUI::getFocus() const {
	return std::make_unique<BorderDefinition>(focus);
}
