#include "songs.hh"

#include "configuration.hh"
#include "fs.hh"
#include "song.hh"
#include "database.hh"
#include "i18n.hh"
#include "profiler.hh"

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <libxml++/libxml++.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

Songs::Songs(Database & database, std::string const& songlist): m_songlist(songlist), math_cover(), m_typeFilter(), m_database(database), m_order(), m_dirty(false), m_loading(false) {
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
	Profiler prof("Song loading took");
	Paths paths = getPathsConfig("system/path_songs");
	for (Paths::iterator it = paths.begin(); m_loading && it != paths.end(); ++it) {
		try {
			if (!fs::is_directory(*it)) { m_debug << ">>> Not scanning: " << *it << " (no such directory)" << std::endl; continue; }
			m_debug << ">>> Scanning " << *it << std::endl;
			size_t count = m_songs.size();
			reload_internal(*it);
			size_t diff = m_songs.size() - count;
			if (diff > 0 && m_loading) m_debug << diff << " songs loaded" << std::endl;
		} catch (std::exception& e) {
			m_debug << ">>> Error scanning " << *it << ": " << e.what() << std::endl;
		}
	}
	prof("total");
	if (m_loading) dumpSongs_internal(); // Dump the songlist to file (if requested)
	m_loading = false;
}

void Songs::reload_internal(fs::path const& parent) {
	namespace fs = fs;
	if (std::distance(parent.begin(), parent.end()) > 20) { m_debug << ">>> Not scanning: " << parent.string() << " (maximum depth reached, possibly due to cyclic symlinks)" << std::endl; return; }
	try {
		boost::regex expression("(.*\\.txt|^song\\.ini|.*\\.sm)$", boost::regex_constants::icase);
		boost::cmatch match;
		for (fs::directory_iterator dirIt(parent), dirEnd; m_loading && dirIt != dirEnd; ++dirIt) {
			fs::path p = dirIt->path();
			if (fs::is_directory(p)) { reload_internal(p); continue; }
			std::string name = p.leaf(); // File basename (notes.txt)
			std::string path = p.directory_string(); // Path without filename
			path.erase(path.size() - name.size());
			if (!regex_match(name.c_str(), match, expression)) continue;
			try {
				boost::shared_ptr<Song>s(new Song(path, name));
				s->randomIdx = rand();
				boost::mutex::scoped_lock l(m_mutex);
				m_songs.push_back(s);
				m_dirty = true;
			} catch (SongParserException& e) {
				if (e.silent()) continue;
				// Construct error message
				m_debug << "-!- Error in " << path << "\n    " << name;
				if (e.line()) m_debug << " line " << e.line();
				m_debug << ": " << e.what() << std::endl;
			}
		}
	} catch (std::exception const& e) {
		m_debug << "Error accessing " << parent << e.what() << std::endl;
	}
}

// Make std::find work with shared_ptrs and regular pointers
static bool operator==(boost::shared_ptr<Song> const& a, Song const* b) { return a.get() == b; }

/// restore selection
class Songs::RestoreSel {
	Songs& m_s;
	Song const* m_sel;
  public:
	/// constructor
	RestoreSel(Songs& s): m_s(s), m_sel(s.empty() ? NULL : &s.current()) {}
	/// resets song to given song
	void reset(Song const* song = NULL) { m_sel = song; }
	~RestoreSel() {
		int pos = 0;
		if (m_sel) {
			SongVector& f = m_s.m_filtered;
			SongVector::iterator it = std::find(f.begin(), f.end(), m_sel);
			m_s.math_cover.setTarget(0, 0);
			if (it != f.end()) pos = it - f.begin();
		}
		m_s.math_cover.setTarget(pos, m_s.size());
	}
};

void Songs::update() {
	if (m_dirty) filter_internal(); // Update with newly loaded songs
	// A hack to move to the first song when the song screen is entered the first time
	static bool first = true;
	if (first) { first = false; math_cover.setTarget(0, 0); math_cover.setTarget(0, size()); }
}

void Songs::setFilter(std::string const& val) {
	if (m_filter == val) return;
	m_filter = val;
	filter_internal();
}

void Songs::setTypeFilter(unsigned char filter) {
	if (m_typeFilter == filter) return;
	m_typeFilter = filter;
	filter_internal();
}

void Songs::filter_internal() {
	boost::mutex::scoped_lock l(m_mutex);
	// Print messages when loading has finished
	if (!m_loading) {
		std::clog << m_debug.str();
		m_debug.str(""); m_debug.clear();
	}
	m_dirty = false;
	RestoreSel restore(*this);
	try {
		SongVector filtered;
		for (SongVector::const_iterator it = m_songs.begin(); it != m_songs.end(); ++it) {
			Song& s = **it;
			if ((m_typeFilter & 1) && !s.hasDance()) continue;
			if ((m_typeFilter & 2) && !s.hasDrums()) continue;
			if ((m_typeFilter & 4) && !s.hasGuitars()) continue;
			if ((m_typeFilter & 8) && !s.hasVocals()) continue;
			if (regex_search(s.strFull(), boost::regex(m_filter, boost::regex_constants::icase))) filtered.push_back(*it);
		}
		m_filtered.swap(filtered);
	} catch (...) {
		SongVector(m_songs.begin(), m_songs.end()).swap(m_filtered);  // Invalid regex => copy everything
	}
	math_cover.reset();
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
    template <typename T> CmpByField<T> comparator(T Song::*field) { return CmpByField<T>(field); }

	std::string pathtrim(std::string path) {
		std::string::size_type pos = path.rfind('/', path.size() - 1);
		pos = path.rfind('/', pos - 1);
		pos = path.rfind('/', pos - 1);
		if (pos == std::string::npos) pos = 0; else ++pos;
		return path.substr(pos, path.size() - pos - 1);
	}

	static const int orders = 7;

}

std::string Songs::sortDesc() const {
	std::string str = "";
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
	if (!empty()) {
		if (m_order == 3) str += " (" + current().edition + ")";
		if (m_order == 4) str += " (" + current().genre + ")";
		if (m_order == 5) str += " (" + pathtrim(current().path) + ")";
		if (m_order == 6) str += " (" + current().language + ")";
	}
	return str;
}

void Songs::sortChange(int diff) {
	m_order = (m_order + diff) % orders;
	if (m_order < 0) m_order += orders;
	RestoreSel restore(*this);
	sort_internal();
}

void Songs::sort_internal() {
	switch (m_order) {
	  case 0: std::stable_sort(m_filtered.begin(), m_filtered.end(), comparator(&Song::randomIdx)); break;
	  case 1: std::sort(m_filtered.begin(), m_filtered.end(), comparator(&Song::collateByTitle)); break;
	  case 2: std::sort(m_filtered.begin(), m_filtered.end(), comparator(&Song::collateByArtist)); break;
	  case 3: std::sort(m_filtered.begin(), m_filtered.end(), comparator(&Song::edition)); break;
	  case 4: std::sort(m_filtered.begin(), m_filtered.end(), comparator(&Song::genre)); break;
	  case 5: std::sort(m_filtered.begin(), m_filtered.end(), comparator(&Song::path)); break;
	  case 6: std::sort(m_filtered.begin(), m_filtered.end(), comparator(&Song::language)); break;
	  default: throw std::logic_error("Internal error: unknown sort order in Songs::sortChange");
	}
}

namespace {
	void dumpCover(xmlpp::Element* song, Song const& s, size_t num) {
		try {
			std::string ext = s.cover.substr(s.cover.rfind('.'));
			fs::path cover = s.path + s.cover;
			if (fs::exists(cover)) {
				std::string coverlink = "covers/" + (boost::format("%|04|") % num).str() + ext;
				if (fs::is_symlink(coverlink)) fs::remove(coverlink);
				create_symlink(cover, coverlink);
				song->add_child("cover")->set_child_text(coverlink);
			}
		} catch (std::exception& e) {
			std::cerr << "Songlist error handling cover image: " << e.what() << std::endl;
		}
	}
	template <typename SongVector> void dumpXML(SongVector const& svec, std::string const& filename) {
		xmlpp::Document doc;
		xmlpp::Element* songlist = doc.create_root_node("songlist");
		songlist->set_attribute("size", boost::lexical_cast<std::string>(svec.size()));
		for (size_t i = 0; i < svec.size(); ++i) {
			Song const& s = *svec[i];
			xmlpp::Element* song = songlist->add_child("song");
			song->set_attribute("num", boost::lexical_cast<std::string>(i + 1));
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
	std::sort(svec.begin(), svec.end(), comparator(&Song::collateByArtist));
	fs::path coverpath = fs::path(m_songlist) / "covers";
	fs::create_directory(coverpath);
	dumpXML(svec, m_songlist + "/songlist.xml");
}

