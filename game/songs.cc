#include "songs.hh"

#include "configuration.hh"
#include "fs.hh"
#include "song.hh"
#include "database.hh"
#include "i18n.hh"
#include "profiler.hh"
#include "libxml++-impl.hh"
#include "unicode.hh"
#include "platform.hh"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <stdexcept>

#include "fs.hh"
#include <boost/format.hpp>
#include <unicode/stsearch.h>

#ifdef USE_WEBSERVER
#include <cpprest/json.h>
#endif

Songs::Songs(Database & database, std::string const& songlist): m_songlist(songlist), m_database(database), m_order(config["songs/sort-order"].i()) {
	m_updateTimer.setTarget(getInf()); // Using this as a simple timer counting seconds
	reload();
}

Songs::~Songs() {
	m_loading = false; // Terminate song loading if currently in progress
	m_thread->join();
}

void Songs::reload() {
	if (m_loading) return;
	if (doneLoading == true) {
		doneLoading = false;
		displayedAlert = false;
	}
	// Run loading thread
	m_loading = true;
	if (m_thread) m_thread->join();
	m_thread = std::make_unique<std::thread>([this]{ reload_internal(); });
}

void Songs::reload_internal() {
	{
		std::lock_guard<std::mutex> l(m_mutex);
		m_songs.clear();
		m_dirty = true;
	}
	LoadCache();
	std::clog << "songs/notice: Done loading the cache. You now have " << m_songs.size() << " songs in your list." << std::endl;
	std::clog << "songs/notice: Starting to load all songs from disk, to update the cache." << std::endl;
	Profiler prof("songloader");
	Paths systemSongs = getPathsConfig("paths/system-songs");
	Paths paths = getPathsConfig("paths/songs");
	paths.insert(paths.begin(), systemSongs.begin(), systemSongs.end());

	for (auto it = paths.begin(); m_loading && it != paths.end(); ++it) { //loop through stored directories from config
		try {
			if (!fs::is_directory(*it)) { std::clog << "songs/info: >>> Not scanning: " << *it << " (no such directory)\n"; continue; }
			std::clog << "songs/info: >>> Scanning " << *it << std::endl;
			size_t count = m_songs.size();
			reload_internal(*it);
			size_t diff = m_songs.size() - count;
			if (diff > 0 && m_loading) std::clog << "songs/info: " << diff << " songs loaded\n";
		} catch (std::exception& e) {
			std::clog << "songs/error: >>> Error scanning " << *it << ": " << e.what() << '\n';
		}
	}
	prof("total");
	if (m_loading) dumpSongs_internal(); // Dump the songlist to file (if requested)
	std::clog << std::flush;
	m_loading = false;
	std::clog << "songs/notice: Done Loading. Loaded " << m_songs.size() << " Songs." << std::endl;
	CacheSonglist();
	std::clog << "songs/notice: Done Caching." << std::endl;
	doneLoading = true;
}

#ifdef USE_WEBSERVER
void Songs::LoadCache() {
	fs::path songsMetaFile = getCacheDir() / "Songs-Metadata.json";
	std::ifstream file(songsMetaFile.string());
	web::json::value jsonRoot;
    if (file)
    {
    	try {
	        std::stringstream buffer;
	        buffer << file.rdbuf();
	        file.close();
	        jsonRoot = web::json::value::parse(buffer);
    	} catch(std::exception const& e) {
			std::clog << "songs/error: " << e.what() << std::endl;
			return;
    	}
    } else {
    	std::clog << "songs/info: Could not open songs meta cache file " << songsMetaFile.string() << std::endl;
    	return;
    }

	Paths systemSongs = getPathsConfig("paths/system-songs");
	Paths localPaths = getPathsConfig("paths/songs");
	localPaths.insert(localPaths.begin(), systemSongs.begin(), systemSongs.end());

	std::vector<std::string> userSongs;
	for(const fs::path& userSong : localPaths) {
		userSongs.push_back(userSong.string());
	}

    for(auto const& song : jsonRoot.as_array()) {
    	STAT buffer;
    	auto songPath = song.at("TxtFile").as_string();
    	auto isSongPathInConfiguredPaths = std::find_if(
                                                        userSongs.begin(), 
                                                        userSongs.end(), 
														[songPath](const std::string& userSongItem) { 
															return songPath.find(userSongItem) != std::string::npos;
														 }) != userSongs.end();
    	if(_STAT(songPath.c_str(), &buffer) == 0 && isSongPathInConfiguredPaths) {
    		std::shared_ptr<Song> realSong(new Song(song));
    		m_songs.push_back(realSong);
    	}  	
    }
}

void Songs::CacheSonglist() {
    web::json::value jsonRoot = web::json::value::array();
    auto i = 0;
	for (auto const& song : m_songs)
    {  
        web::json::value songObject = web::json::value::object();
        if(!song->path.string().empty()) {
        	songObject["TxtFileFolder"] = web::json::value::string(song->path.string());
        }
        if(!song->filename.string().empty()) {
        	songObject["TxtFile"] = web::json::value::string(song->filename.string());
        }
        if(!song->title.empty()) {
        	songObject["Title"] = web::json::value::string(song->title);
    	}
		if(!song->artist.empty()) {
        	songObject["Artist"] = web::json::value::string(song->artist);
    	}
        if(!song->edition.empty()) {
        	songObject["Edition"] = web::json::value::string(song->edition);
    	}
    	if(!song->language.empty()) {
        	songObject["Language"] = web::json::value::string(song->language);
        }
        if(!song->creator.empty()) {
        	songObject["Creator"] = web::json::value::string(song->creator);
    	}
    	if(!song->genre.empty()) {
        	songObject["Genre"] = web::json::value::string(song->genre);
    	}
    	if(!song->cover.string().empty()) {
        	songObject["Cover"] = web::json::value::string(song->cover.string());
    	}
    	if(!song->background.string().empty()) {
	        songObject["Background"] = web::json::value::string(song->background.string());
	    }
    	if(!song->music[TrackName::BGMUSIC].string().empty()) {
	        songObject["SongFile"] = web::json::value::string(song->music[TrackName::BGMUSIC].string());
	    }
    	if(!song->midifilename.string().empty()) {
	        songObject["MidFile"] = web::json::value::string(song->midifilename.string());
	    }
    	if(!song->video.string().empty()) {
	        songObject["VideoFile"] = web::json::value::string(song->video.string());
	    }
    	if(!std::isnan(song->start)) {
	        songObject["Start"] = web::json::value(song->start);
	    }
    	if(!std::isnan(song->videoGap)) {
	        songObject["VideoGap"] = web::json::value(song->videoGap);
	    }
    	if(!std::isnan(song->preview_start)) {
	        songObject["PreviewStart"] = web::json::value::number(song->preview_start);
	    }
    	if(!song->music[TrackName::LEAD_VOCAL].string().empty()) {
	        songObject["Vocals"] = web::json::value::string(song->music[TrackName::LEAD_VOCAL].string());
	    }
        if(!song->music[TrackName::PREVIEW].string().empty()) {
	        songObject["Preview"] = web::json::value::string(song->music[TrackName::PREVIEW].string());
        }
        if(!song->music[TrackName::GUITAR].string().empty()) {
	        songObject["Guitar"] = web::json::value::string(song->music[TrackName::GUITAR].string());
        }
        if(!song->music[TrackName::BASS].string().empty()) {
	        songObject["Bass"] = web::json::value::string(song->music[TrackName::BASS].string());
        }
        if(!song->music[TrackName::DRUMS].string().empty()) {
	        songObject["Drums"] = web::json::value::string(song->music[TrackName::DRUMS].string());
        }
        if(!song->music[TrackName::KEYBOARD].string().empty()) {
	        songObject["Keyboard"] = web::json::value::string(song->music[TrackName::KEYBOARD].string());
        }
        if(!song->music[TrackName::GUITAR_COOP].string().empty()) {
	        songObject["Guitar_coop"] = web::json::value::string(song->music[TrackName::GUITAR_COOP].string());
        }
        if(!song->music[TrackName::GUITAR_RHYTHM].string().empty()) {
	        songObject["Guitar_Rhythm"] = web::json::value::string(song->music[TrackName::GUITAR_RHYTHM].string());
        }

    	double duration = song->getDurationSeconds();
    	if(!std::isnan(duration)) {
	    	songObject["Duration"] = web::json::value::number(duration);
	    }
	    if (!song->m_bpms.empty()) {
			songObject["BPM"] = web::json::value::number(15 / song->m_bpms.front().step);
		}
			
		// Cache songtype also.
		if(song->hasVocals()) {
			uint32_t vocals = song->vocalTracks.size();
        	songObject["VocalTracks"] = web::json::value::number(vocals);
    	}
		if(song->hasKeyboard()) {
        	songObject["KeyboardTracks"] = web::json::value::number(1);
    	}
		if(song->hasDrums()) {
        	songObject["DrumTracks"] = web::json::value::number(1);
    	}
		if(song->hasDance()) {
			uint32_t dance = song->danceTracks.size();
        	songObject["DanceTracks"] = web::json::value::number(dance);
    	}
		if(song->hasGuitars()) {
			uint32_t guitars = song->instrumentTracks.size() - song->hasDrums() - song->hasKeyboard();
        	songObject["GuitarTracks"] = web::json::value::number(guitars);
    	}
	    if(songObject != web::json::value::object()) {
        	jsonRoot[i] = songObject;
        	i++;
    	}
	}

	fs::path cacheDir = getCacheDir() / "Songs-Metadata.json";

	try {
		std::stringstream stream;
		jsonRoot.serialize(stream);
		std::ofstream outFile(cacheDir.string());
    	outFile << stream.rdbuf();
    	outFile.close();
	} catch (std::exception const& e) {
		std::clog << "songs/error: Could not save " + cacheDir.string() + ": " + e.what() << std::endl;
		return;
	}
}
#else 
void Songs::LoadCache() { }
void Songs::CacheSonglist() { }
#endif

void Songs::reload_internal(fs::path const& parent) {
	if (std::distance(parent.begin(), parent.end()) > 20) { std::clog << "songs/info: >>> Not scanning: " << parent.string() << " (maximum depth reached, possibly due to cyclic symlinks)\n"; return; }
	try {
		std::regex expression(R"((\.txt|^song\.ini|^notes\.xml|\.sm)$)", std::regex_constants::icase);
		for (const auto &dir : fs::directory_iterator(parent)) { //loop through files
			fs::path p = dir.path();
			if (fs::is_directory(p)) { reload_internal(p); continue; } //if the file is a folder redo this function with this folder as path
			if (!regex_search(p.filename().string(), expression)) continue; //if the folder does not contain any of the requested files, ignore it
			try { //found song file, make a new song with it.
				auto it = std::find_if(m_songs.begin(), m_songs.end(), [p](std::shared_ptr<Song> n) {
					return n->filename == p;
				});
				auto alreadyInCache = it != m_songs.end();

				if(alreadyInCache) { 
					continue;
				}

				std::clog << "songs/notice: Found song which was not in the cache: " << p.string() << std::endl;

				std::shared_ptr<Song>s(new Song(p.parent_path(), p));
				std::lock_guard<std::mutex> l(m_mutex);
				int AdditionalFileIndex = -1;
				for(unsigned int i = 0; i< m_songs.size(); i++) {
					if(s->filename.extension() != m_songs[i]->filename.extension() && s->filename.stem() == m_songs[i]->filename.stem() &&
							s->title == m_songs[i]->title && s->artist == m_songs[i]->artist) {
						std::clog << "songs/info: >>> Found additional song file: " << s->filename << " for: " << m_songs[i]->filename << std::endl;
						AdditionalFileIndex = i;
					}
				}				

				if(AdditionalFileIndex > 0) { //TODO: add it to existing song
					std::clog << "songs/info: >>> not yet implemented " << std::endl;
					s->getDurationSeconds();
					m_songs.push_back(s); // will make it appear double!!
				} else {
					s->getDurationSeconds();
					m_songs.push_back(s); //put it in the database
				}
				m_dirty = true;
			} catch (SongParserException& e) {
				std::clog << e;
			}
		}
	} catch (std::exception const& e) {
		std::clog << "songs/error: Error accessing " << parent << ": " << e.what() << '\n';
	}
}

// Make std::find work with shared_ptrs and regular pointers
static bool operator==(std::shared_ptr<Song> const& a, Song const* b) { return a.get() == b; }

/// Store currently selected song on construction and restore the selection on destruction
/// Assumes that m_filtered has been modified and finds the old selection by pointer value.
/// Sets up math_cover so that the old selection is restored if possible, otherwise the first song is selected.
class Songs::RestoreSel {
	Songs& m_s;
	Song const* m_sel;
  public:
	/// constructor
	RestoreSel(Songs& s): m_s(s), m_sel(s.empty() ? nullptr : &s.current()) {}
	/// resets song to given song
	void reset(Song const* song = nullptr) { m_sel = song; }
	~RestoreSel() {
		int pos = 0;
		if (m_sel) {
			SongVector& f = m_s.m_filtered;
			auto it = std::find(f.begin(), f.end(), m_sel);
			m_s.math_cover.reset();
			if (it != f.end()) pos = it - f.begin();
		}
		m_s.math_cover.setTarget(pos, m_s.size());
	}
};

void Songs::update() {
	if (m_dirty && m_updateTimer.get() > 0.5) filter_internal(); // Update with newly loaded songs
	// A hack to move to the first song when the song screen is entered the first time
	static bool first = true;
	if (first) { first = false; math_cover.reset(); math_cover.setTarget(0, size()); }
}

void Songs::setFilter(std::string const& val) {
	if (m_filter == val) return;
	m_filter = val;
	filter_internal();
}

void Songs::filter_internal() {
	m_updateTimer.setValue(0.0);
	std::lock_guard<std::mutex> l(m_mutex);
	m_dirty = false;
	RestoreSel restore(*this);
	try {
		SongVector filtered;
		// if filter text is blank and no type filter is set, just display all songs.
		if (m_filter == std::string() && m_type == 0) filtered = m_songs;
		else {
			std::string charset = UnicodeUtil::getCharset(m_filter);
			icu::UnicodeString filter = ((charset == "UTF-8") ? icu::UnicodeString::fromUTF8(m_filter) : icu::UnicodeString(m_filter.c_str(), charset.c_str()));
			UErrorCode icuError = U_ZERO_ERROR;
			
			std::copy_if (m_songs.begin(), m_songs.end(), std::back_inserter(filtered), [&](std::shared_ptr<Song> it){
			// Filter by type first.	
				if (m_type == 1 && !(*it).hasDance()) return false;
				if (m_type == 2 && !(*it).hasVocals()) return false;
				if (m_type == 3 && !(*it).hasDuet()) return false;
				if (m_type == 4 && !(*it).hasGuitars()) return false;
				if (m_type == 5 && !(*it).hasDrums() && !(*it).hasKeyboard()) return false;
				if (m_type == 6 && (!(*it).hasVocals() || !(*it).hasGuitars() || (!(*it).hasDrums() && !(*it).hasKeyboard()))) return false;
				
		  // If search is not empty, filter by search term.	
				if (!m_filter.empty()) {
					icu::StringSearch search = icu::StringSearch(filter, icu::UnicodeString::fromUTF8((*it).strFull()), &UnicodeUtil::m_dummyCollator, nullptr, icuError);
					return (search.first(icuError) != USEARCH_DONE);
					}
					
		// If we still haven't returned, it must be a type match with an empty search string.			
				return true;
			});
		}
		m_filtered.swap(filtered);
	} catch (...) {
		SongVector(m_songs.begin(), m_songs.end()).swap(m_filtered);  // Invalid regex => copy everything
	}
	sort_internal();
}

namespace {

	/// A functor that compares songs based on a selected member field of them.
	template<typename Field> class CmpByField {
		Field Song::* m_field;
	  public:
		/** @param field a pointer to the field to use (pointer to member) **/
		CmpByField(Field Song::* field): m_field(field) {}
		/// Compare left < right
		bool operator()(Song const& left , Song const& right) {
			return left.*m_field < right.*m_field;
		}
		/// Compare *left < *right
		bool operator()(std::shared_ptr<Song> const& left, std::shared_ptr<Song> const& right) {
			return operator()(*left, *right);
		}
	};
	
	template<> class CmpByField<std::string> {
		std::string Song::* m_field;
	  public:
		/** @param field a pointer to the field to use (pointer to member) **/
		CmpByField(std::string Song::* field): m_field(field) {}
		/// Compare left < right
		bool operator()(Song const& left , Song const& right) {
			icu::UnicodeString leftVal = icu::UnicodeString::fromUTF8(left.*m_field);
			icu::UnicodeString rightVal = icu::UnicodeString::fromUTF8(right.*m_field);
			UErrorCode sortError = U_ZERO_ERROR;
			UCollationResult result = UnicodeUtil::m_sortCollator.compare(leftVal, rightVal, sortError);
			if (U_SUCCESS(sortError)) {
			return (result == UCOL_LESS);
			}
			else {
			throw std::runtime_error("unicode/error: Sorting comparison error in CmpByField<std::string> ");
			}
		}
		/// Compare *left < *right
		bool operator()(std::shared_ptr<Song> const& left, std::shared_ptr<Song> const& right) {
			return operator()(*left, *right);
		}
	};

	/// A helper for easily constructing CmpByField objects
	template <typename T> CmpByField<T> customComparator(T Song::*field) { return CmpByField<T>(field); }
	static const int types = 7, orders = 7;

}

std::string Songs::typeDesc() const {
	switch (m_type) {
		case 0: return _("show all songs");
		case 1: return _("has dance");
		case 2: return _("has vocals");
		case 3: return _("has duet");
		case 4: return _("has guitar");
		case 5: return _("drums or keytar");
		case 6: return _("full band");
	}
	throw std::logic_error("Internal error: unknown type filter in Songs::typeDesc");
}

void Songs::typeChange(int diff) {
	if (diff == 0) m_type = 0;
	else {
		m_type = (m_type + diff) % types;
		if (m_type < 0) m_type += types;
	}
	filter_internal();
}

void Songs::typeCycle(int cat) {
	static const int categories[types] = { 0, 1, 2, 2, 3, 3, 4 };
	// Find the next matching category
	int type = 0;
	for (int t = (categories[m_type] == cat ? m_type + 1 : 0); t < types; ++t) {
		if (categories[t] == cat) { type = t; break; }
	}
	m_type = type;
	filter_internal();
}

std::string Songs::sortDesc() const {
	std::string str;
	switch (m_order) {
	  case 0: str = _("random order"); break;
	  case 1: str = _("sorted by song"); break;
	  case 2: str = _("sorted by artist"); break;
	  case 3: str = _("sorted by edition"); break;
	  case 4: str = _("sorted by genre"); break;
	  case 5: str = _("sorted by path"); break;
	  case 6: str = _("sorted by language"); break;
	  default: throw std::logic_error("Internal error: unknown sort order in Songs::sortDesc");
	}
	return str;
}

void Songs::sortChange(int diff) {
	m_order = (m_order + diff) % orders;
	if (m_order < 0) m_order += orders;
	RestoreSel restore(*this);
	config["songs/sort-order"].i() = m_order;
	switch (m_order) {
		case 1:
		 [[fallthrough]];
		case 2:
		 [[fallthrough]];
		case 3:
		 [[fallthrough]];
		case 4:
		 [[fallthrough]];
		case 6:
			UErrorCode collatorError = U_ZERO_ERROR;
			UnicodeUtil::m_sortCollator.setAttribute(UCOL_STRENGTH, (config["game/case-sorting"].b()) ? UCOL_TERTIARY : UCOL_SECONDARY, collatorError);
			if (U_FAILURE(collatorError)) {
				std::clog << "sorting/error: Unable to change collator strength." << std::endl;
			}
			break;		
		}
	sort_internal();
	writeConfig(false);
}

void Songs::sortSpecificChange(int sortOrder, bool descending) {
	if(sortOrder < 0) {
		m_order = 0;
	} else if(sortOrder <= 6) {
		m_order = sortOrder;
	} else {
		m_order = 0;
	}
	RestoreSel restore(*this);
	config["songs/sort-order"].i() = m_order;
	sort_internal(descending);
}

void Songs::sort_internal(bool descending) {
	if(descending) {
		switch (m_order) {
		  case 0: std::stable_sort(m_filtered.begin(), m_filtered.end(), customComparator(&Song::randomIdx)); break;
		  case 1: std::sort(m_filtered.rbegin(), m_filtered.rend(), customComparator(&Song::collateByTitle)); break;
		  case 2: std::sort(m_filtered.rbegin(), m_filtered.rend(), customComparator(&Song::collateByArtist)); break;
		  case 3: std::sort(m_filtered.rbegin(), m_filtered.rend(), customComparator(&Song::edition)); break;
		  case 4: std::sort(m_filtered.rbegin(), m_filtered.rend(), customComparator(&Song::genre)); break;
		  case 5: std::sort(m_filtered.rbegin(), m_filtered.rend(), customComparator(&Song::path)); break;
		  case 6: std::sort(m_filtered.rbegin(), m_filtered.rend(), customComparator(&Song::language)); break;
		  default: throw std::logic_error("Internal error: unknown sort order in Songs::sortChange");
		}
	} else {
		switch (m_order) {
		  case 0: std::stable_sort(m_filtered.begin(), m_filtered.end(), customComparator(&Song::randomIdx)); break;
		  case 1: std::sort(m_filtered.begin(), m_filtered.end(), customComparator(&Song::collateByTitle)); break;
		  case 2: std::sort(m_filtered.begin(), m_filtered.end(), customComparator(&Song::collateByArtist)); break;
		  case 3: std::sort(m_filtered.begin(), m_filtered.end(), customComparator(&Song::edition)); break;
		  case 4: std::sort(m_filtered.begin(), m_filtered.end(), customComparator(&Song::genre)); break;
		  case 5: std::sort(m_filtered.begin(), m_filtered.end(), customComparator(&Song::path)); break;
		  case 6: std::sort(m_filtered.begin(), m_filtered.end(), customComparator(&Song::language)); break;
		  default: throw std::logic_error("Internal error: unknown sort order in Songs::sortChange");
		}
	}
}

namespace {
	void dumpCover(xmlpp::Element* song, Song const& s, size_t num) {
		try {
			std::string ext = s.cover.extension().string();
			if (exists(s.cover)) {
				std::string coverlink = "covers/" + (boost::format("%|04|") % num).str() + ext;
				if (fs::is_symlink(coverlink)) fs::remove(coverlink);
				create_symlink(s.cover, coverlink);
				xmlpp::set_first_child_text(xmlpp::add_child_element(song, "cover"), coverlink);
			}
		} catch (std::exception& e) {
			std::cerr << "Songlist error handling cover image: " << e.what() << std::endl;
		}
	}
	template <typename SongVector> void dumpXML(SongVector const& svec, std::string const& filename) {
		xmlpp::Document doc;
		xmlpp::Element* songlist = doc.create_root_node("songlist");
		songlist->set_attribute("size", std::to_string(svec.size()));
		for (size_t i = 0; i < svec.size(); ++i) {
			Song const& s = *svec[i];
			xmlpp::Element* song = xmlpp::add_child_element(songlist, "song");
			song->set_attribute("num", std::to_string(i + 1));
			xmlpp::Element* collate = xmlpp::add_child_element(song, "collate");
			xmlpp::set_first_child_text(xmlpp::add_child_element(collate, "artist"), s.collateByArtist);
			xmlpp::set_first_child_text(xmlpp::add_child_element(collate, "title"), s.collateByTitle);
			xmlpp::set_first_child_text(xmlpp::add_child_element(song, "artist"), s.artist);
			xmlpp::set_first_child_text(xmlpp::add_child_element(song, "title"), s.title);
			if (!s.cover.empty()) dumpCover(song, s, i + 1);
		}
		doc.write_to_file_formatted(filename, "UTF-8");
	}
}

void Songs::dumpSongs_internal() const {
	if (m_songlist.empty()) return;
	SongVector svec = m_songs;
	std::sort(svec.begin(), svec.end(), customComparator(&Song::collateByArtist));
	fs::path coverpath = fs::path(m_songlist) / "covers";
	fs::create_directories(coverpath);
	dumpXML(svec, m_songlist + "/songlist.xml");
}

