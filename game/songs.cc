#include "songs.hh"

#include "configuration.hh"
#include "fs.hh"
#include "song.hh"
#include "database.hh"
#include "i18n.hh"
#include "profiler.hh"

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <libxml++/libxml++.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

Songs::Songs(Database & database, std::string const& songlist): m_songlist(songlist), math_cover(), m_database(database), m_type(), m_order(config["songs/sort-order"].i()), m_dirty(false), m_loading(false) {
	m_updateTimer.setTarget(getInf()); // Using this as a simple timer counting seconds
	reload();
}

Songs::~Songs() {
	m_loading = false; // Terminate song loading if currently in progress
	m_thread->join();
}

void Songs::reload() {
	if (m_loading) return;
	// Run loading thread
	m_loading = true;
	m_thread.reset(new boost::thread(boost::bind(&Songs::reload_internal, boost::ref(*this))));
}

void Songs::reload_internal() {
	{
		boost::mutex::scoped_lock l(m_mutex);
		m_songs.clear();
		m_dirty = true;
	}
	Profiler prof("songloader");
	Paths paths = getPathsConfig("paths/songs");
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
	Game* gm = Game::getSingletonPtr();
    //gm->dialog(_("Done Loading!\n Loaded ") + boost::lexical_cast<std::string>(m_songs.size()) + " Songs.");	//crashes on ubuntu 16.10 but not on 16.04
}

void Songs::reload_internal(fs::path const& parent) {
	if (std::distance(parent.begin(), parent.end()) > 20) { std::clog << "songs/info: >>> Not scanning: " << parent.string() << " (maximum depth reached, possibly due to cyclic symlinks)\n"; return; }
	try {
		boost::regex expression(R"((\.txt|^song\.ini|^notes\.xml|\.sm)$)", boost::regex_constants::icase);
		for (fs::directory_iterator dirIt(parent), dirEnd; m_loading && dirIt != dirEnd; ++dirIt) { //loop through files
			fs::path p = dirIt->path();
			if (fs::is_directory(p)) { reload_internal(p); continue; } //if the file is a folder redo this function with this folder as path
			if (!regex_search(p.filename().string(), expression)) continue; //if the folder does not contain any of the requested files, ignore it
			try { //found song file, make a new song with it.
				boost::shared_ptr<Song>s(new Song(p.parent_path(), p));
				s->randomIdx = rand(); //give it a random identifier
				boost::mutex::scoped_lock l(m_mutex);
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
static bool operator==(boost::shared_ptr<Song> const& a, Song const* b) { return a.get() == b; }

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
	boost::mutex::scoped_lock l(m_mutex);
	m_dirty = false;
	RestoreSel restore(*this);
	try {
		SongVector filtered;
		for (SongVector::const_iterator it = m_songs.begin(); it != m_songs.end(); ++it) {
			Song& s = **it;
			// All, Dance, Vocals, Duet, Guitar, Band
			if (m_type == 1 && !s.hasDance()) continue;
			if (m_type == 2 && !s.hasVocals()) continue;
			if (m_type == 3 && !s.hasDuet()) continue;
			if (m_type == 4 && !s.hasGuitars()) continue;
			if (m_type == 5 && !s.hasDrums() && !s.hasKeyboard()) continue;
			if (m_type == 6 && (!s.hasVocals() || !s.hasGuitars() || (!s.hasDrums() && !s.hasKeyboard()))) continue;
			if (regex_search(s.strFull(), boost::regex(m_filter, boost::regex_constants::icase))) filtered.push_back(*it);
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
		bool operator()(boost::shared_ptr<Song> const& left, boost::shared_ptr<Song> const& right) {
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
	sort_internal();
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
				song->add_child("cover")->set_child_text(coverlink);
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
			xmlpp::Element* song = songlist->add_child("song");
			song->set_attribute("num", std::to_string(i + 1));
			xmlpp::Element* collate = song->add_child("collate");
			collate->add_child("artist")->set_child_text(s.collateByArtist);
			collate->add_child("title")->set_child_text(s.collateByTitle);
			song->add_child("artist")->set_child_text(s.artist);
			song->add_child("title")->set_child_text(s.title);
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

