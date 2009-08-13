#include "songs.hh"

#include "fs.hh"
#include "screen.hh"
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <libxml++/libxml++.h>
// #include <tr1/random>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

Songs::Songs(std::string const& songlist): m_songlist(songlist), math_cover(), m_order(), m_dirty(false), m_loading(false), m_needShuffle(false) {
	reload();
}

Songs::~Songs() {
	m_loading = false; // Terminate song loading if currently in progress
	m_thread->join();
}

void Songs::reload() {
	if (m_loading) return;
	// Copy songdirs from config into m_songdirs
	ConfigItem::StringList const& sd = config["system/path_songs"].sl();
	m_songdirs.clear();
	std::transform(sd.begin(), sd.end(), std::inserter(m_songdirs, m_songdirs.end()), pathMangle);
	// Run loading thread
	m_needShuffle = false;
	m_loading = true;
	m_thread.reset(new boost::thread(boost::bind(&Songs::reload_internal, boost::ref(*this))));
}

void Songs::reload_internal() {
	{
		boost::mutex::scoped_lock l(m_mutex);
		m_songs.clear();
		m_dirty = true;
	}
	for (SongDirs::const_iterator it = m_songdirs.begin(); m_loading && it != m_songdirs.end(); ++it) {
		if (!fs::is_directory(*it)) { std::cout << ">>> Not scanning: " << *it << " (no such directory)" << std::endl; continue; }
		std::cout << ">>> Scanning " << *it << std::endl;
		size_t count = m_songs.size();
		reload_internal(*it);
		size_t diff = m_songs.size() - count;
		if (diff > 0 && m_loading) std::cout << diff << " songs loaded" << std::endl;
	}
	if (m_loading) dumpSongs_internal(); // Dump the songlist to file (if requested)
	m_loading = false;
	m_needShuffle = true;  // Force shuffle
}

void Songs::reload_internal(fs::path const& parent) {
	static int randomIdx = 0;
	namespace fs = fs;
	if (std::distance(parent.begin(), parent.end()) > 20) { std::cout << ">>> Not scanning: " << parent.string() << " (maximum depth reached, possibly due to cyclic symlinks)" << std::endl; return; }
	try {
		boost::regex expression("(.*\\.txt|^song\\.ini)$", boost::regex_constants::icase);
		boost::cmatch match;
		for (fs::directory_iterator dirIt(parent), dirEnd; m_loading && dirIt != dirEnd; ++dirIt) {
			fs::path p = dirIt->path();
			if (fs::is_directory(p)) { reload_internal(p); continue; }
			std::string name = p.leaf(); // File basename (notes.txt)
			std::string path = p.directory_string(); // Path without filename
			path.erase(path.size() - name.size());
			if (!regex_match(name.c_str(), match, expression)) continue;
			try {
				Song* s = new Song(path, name);
				s->randomIdx = ++randomIdx; // Not so random during loading, they are shuffled after load is finished
				boost::mutex::scoped_lock l(m_mutex);
				m_songs.push_back(boost::shared_ptr<Song>(s));
				m_dirty = true;
			} catch (SongParserException& e) {
				if (e.silent()) continue;
				std::ostringstream oss;
				oss << "-!- Error in " << path << "\n    " << name;
				if (e.line()) oss << " line " << e.line();
				oss << ": " << e.what() << std::endl;
				std::cerr << oss.str(); // More likely to be atomic when written as one string
			}
		}
	} catch (std::exception const& e) {
		std::cout << "Error accessing " << parent << std::endl;
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
	if (m_dirty) filter_internal();
	// Shuffle the songlist if shuffle is finished and all songs are already filtered
	if (m_needShuffle && !m_dirty) {
		randomize_internal();
		math_cover.setTarget(0, m_songs.size());
		m_needShuffle = false;
	}
}

void Songs::randomize() {
	RestoreSel restore(*this);
	randomize_internal();
}

void Songs::randomize_internal() {
	/* TR1-based random number generation
	namespace rnd = std::tr1;
	rnd::random_device gendev;  // Random number generator (using /dev/urandom usually)
	rnd::mt19937 gen(gendev);  // Make Mersenne Twister random number generator, seeded with random_device.
	for (SongVector::const_iterator it = m_filtered.begin(); it != m_filtered.end(); ++it) (*it)->randomIdx = gen();
	*/
	std::srand(std::time(NULL));
	// Assign the songs randomIdx that is used for sorting in the "random" mode
	for (SongVector::const_iterator it = m_filtered.begin(); it != m_filtered.end(); ++it) (*it)->randomIdx = std::rand();
	m_order = 0;  // Use randomIdx sort mode
	sort_internal();
}

void Songs::setFilter(std::string const& val) {
	if (m_filter == val) return;
	m_filter = val;
	filter_internal();
}

void Songs::filter_internal() {
	boost::mutex::scoped_lock l(m_mutex);
	m_dirty = false;
	RestoreSel restore(*this);
	try {
		SongVector filtered;
		for (SongVector::const_iterator it = m_songs.begin(); it != m_songs.end(); ++it) {
			if (regex_search(it->get()->strFull(), boost::regex(m_filter, boost::regex_constants::icase))) filtered.push_back(*it);
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


    static char const* order[] = {
	    "random order",
	    "by song",
	    "by artist",
	    "by edition",
	    "by genre",
	    "by path",
	    "by language"
    };

    static const int orders = sizeof(order) / sizeof(*order);

}

std::string Songs::sortDesc() const {
	std::string str = order[m_order];
	if (!empty()) {
		if (m_order == 3) str += " (" + current().edition + ")";
		if (m_order == 4) str += " (" + current().genre + ")";
		if (m_order == 5) str += " (" + pathtrim(current().path) + ")";
	}
	return str;
}

void Songs::sortChange(int diff) {
	m_order = (m_order + diff) % orders;
	if (m_order < 0) m_order += orders;
	RestoreSel restore(*this);
	sort_internal();
}

namespace {
	int rnd(int n) { return rand() % n; }
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

