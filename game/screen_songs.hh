#pragma once

#include <boost/scoped_ptr.hpp>
#include "animvalue.hh"
#include "cachemap.hh"
#include "database.hh"
#include "screen.hh"
#include "surface.hh"
#include "textinput.hh"
#include "theme.hh"
#include "video.hh"
#include "joystick.hh"

class Song;
class Audio;
class Songs;

struct ScreenSharedInfo
{
	std::map<std::string,std::string> music;
	std::string songbg;
	std::string video;
	double videoGap;
};

/// song chooser screen
class ScreenSongs : public Screen {
public:
	/// constructor
	ScreenSongs(std::string const& name, Audio& audio, Songs& songs, Database& database);
	void enter();
	void exit();
	void manageSharedKey(input::NavButton nav); ///< same behaviour for jukebox and normal mode
	void manageEvent(SDL_Event event);
	void draw();
	void drawJukebox(); ///< draw the songbrowser in jukebox mode (fullscreen, full previews, ...)

protected:
	void drawInstruments(Song const& song, Dimensions const& dim, float alpha = 1.0f) const;
	void drawMultimedia();
	void updateMultimedia(Song& song, ScreenSharedInfo& info);
	void stopMultimedia(ScreenSharedInfo& info);

	Audio& m_audio;
	Songs& m_songs;
	Database& m_database;
	boost::scoped_ptr<Surface> m_songbg;
	boost::scoped_ptr<Surface> m_songbg_default;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<ThemeSongs> theme;
	std::map<std::string,std::string> m_playing;
	std::map<std::string,std::string> m_playReq;
	AnimValue m_playTimer;
	TextInput m_search;
	boost::scoped_ptr<Surface> m_singCover;
	boost::scoped_ptr<Surface> m_instrumentCover;
	boost::scoped_ptr<Surface> m_bandCover;
	boost::scoped_ptr<Surface> m_danceCover;
	boost::scoped_ptr<Texture> m_instrumentList;
	Cachemap<std::string, Surface> m_covers;
	bool m_jukebox;
	bool show_hiscores;
	int hiscore_start_pos;
};
