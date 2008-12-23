#include "songs.hh"

#include "screen.hh"
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <libxml++/libxml++.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

Songs::Songs(std::set<std::string> const& songdirs, std::string const& songlist): m_songdirs(songdirs), m_songlist(songlist), math_cover(), m_order(), m_dirty(false), m_loading(false) {
	reload();
}

Songs::~Songs() {
	m_loading = false; // Terminate song loading if currently in progress
	m_thread->join();
}

void Songs::reload() {
	if (m_loading) return;
	m_loading = true;
	m_thread.reset(new boost::thread(boost::bind(&Songs::reload_internal, boost::ref(*this))));
}

void Songs::reload_internal() {
	{
		boost::mutex::scoped_lock l(m_mutex);
		m_songs.clear();
		m_dirty = true;
	}
	for (std::set<std::string>::const_iterator it = m_songdirs.begin(); m_loading && it != m_songdirs.end(); ++it) {
		if (!boost::filesystem::is_directory(*it)) { std::cout << ">>> Not scanning: " << *it << " (no such directory)" << std::endl; continue; }
		std::cout << ">>> Scanning " << *it << std::endl;
		size_t count = m_songs.size();
		reload_internal(*it);
		size_t diff = m_songs.size() - count;
		if (diff > 0) std::cout << diff << " songs loaded" << std::endl;
	}
	dumpSongs_internal(); // Dump the songlist to file (if requested)
	m_loading = false;
	m_dirty = true;  // Force shuffle
}

void Songs::reload_internal(boost::filesystem::path const& parent) {
	namespace fs = boost::filesystem;
	if (std::distance(parent.begin(), parent.end()) > 20) { std::cout << ">>> Not scanning: " << parent.string() << " (maximum depth reached, possibly due to cyclic symlinks)" << std::endl; return; }
	try {
		boost::regex expression(".*\\.[Tt][Xx][Tt]$");
		boost::cmatch match;
		for (fs::directory_iterator dirIt(parent), dirEnd; m_loading && dirIt != dirEnd; ++dirIt) {
			fs::path p = dirIt->path();
			if (fs::is_directory(p)) { reload_internal(p); continue; }
			std::string name = p.leaf(); // File basename (notes.txt)
			std::string path = p.directory_string(); // Path without filename
			path.erase(path.size() - name.size());
			if (name.size() < 5 || !regex_match(name.substr(name.size() - 4).c_str(),match,expression)) continue;
			try {
				Song* s = new Song(path, name);
				boost::mutex::scoped_lock l(m_mutex);
				m_songs.push_back(boost::shared_ptr<Song>(s));
				m_dirty = true;
			} catch (SongParserException& e) {
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

void Songs::randomize() {
	RestoreSel restore(*this);
	m_order = 0;
	sort_internal();
}

void Songs::setFilter(std::string const& val) {
	if (m_filter == val) return;
	m_filter = val;
	filter_internal();
}

void Songs::filter_internal() {
	boost::mutex::scoped_lock l(m_mutex);
	RestoreSel restore(*this);
	try {
		SongVector filtered;
		for (SongVector::const_iterator it = m_songs.begin(); it != m_songs.end(); ++it) {
			if (regex_search(it->get()->strFull(), boost::regex(m_filter ,boost::regex_constants::icase))) filtered.push_back(*it);
		}
		m_filtered.swap(filtered);
	} catch (...) {
		SongVector(m_songs.begin(), m_songs.end()).swap(m_filtered);  // Invalid regex => copy everything
	}
	math_cover.setTarget(0, 0);
	sort_internal();
	if (m_dirty && !m_loading) restore.reset();
	m_dirty = false;
}

#define STRLT_RET(lhs, rhs) { std::string l_ = Glib::ustring(lhs).casefold_collate_key(), r_ = Glib::ustring(rhs).casefold_collate_key(); if (l_ != r_) return l_ < r_; }
/// class to compare by field XXX
class CmpByField {
	std::string Song::* m_field;
  public:
	/// constructor
	CmpByField(std::string Song::* field): m_field(field) {}
	/// compare left and right song
	bool operator()(Song const& left , Song const& right) {
		STRLT_RET(left.*m_field, right.*m_field);
		return false;
	}
	/// compare left and right song
	bool operator()(boost::shared_ptr<Song> const& left, boost::shared_ptr<Song> const& right) {
		return operator()(*left, *right);
	}
};
#undef STRLT_RET

namespace {
	std::string pathtrim(std::string path) {
		std::string::size_type pos = path.rfind('/', path.size() - 1);
		pos = path.rfind('/', pos - 1);
		pos = path.rfind('/', pos - 1);
		if (pos == std::string::npos) pos = 0; else ++pos;
		return path.substr(pos, path.size() - pos - 1);
	}
}

static char const* order[] = {
	"random order",
	"by song",
	"by artist",
	"by edition",
	"by genre",
	"by path"
};

static const int orders = sizeof(order) / sizeof(*order);

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
	  case 0:
		if (!m_loading) {
			srand(time(NULL));
			std::random_shuffle(m_filtered.begin(), m_filtered.end(), rnd);
		}
		break;
	  case 1: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::collateByTitle)); break;
	  case 2: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::collateByArtist)); break;
	  case 3: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::edition)); break;
	  case 4: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::genre)); break;
	  case 5: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::path)); break;
	  default: throw std::logic_error("Internal error: unknown sort order in Songs::sortChange");
	}
}

namespace {
	template <typename SongVector> void dumpXML(SongVector const& s, std::string const& title, std::string const& filename) {
		xmlpp::Document doc;
		doc.set_internal_subset("html", "-//W3C//DTD XHTML 1.1//EN", "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd");
		xmlpp::Element* html = doc.create_root_node("html", "http://www.w3.org/1999/xhtml");
		xmlpp::Element* head = html->add_child("head");
		head->add_child("title")->set_child_text(title);
		xmlpp::Element* body = html->add_child("body");
		xmlpp::Element* table = body->add_child("table");
		xmlpp::Element* headtr = table->add_child("tr");
		headtr->add_child("th")->set_child_text("Artist");
		headtr->add_child("th")->set_child_text("Title");
		for (typename SongVector::const_iterator it = s.begin(); it != s.end(); ++it) {
			xmlpp::Element* tr = table->add_child("tr");
			tr->add_child("td")->set_child_text((*it)->artist);
			tr->add_child("td")->set_child_text((*it)->title);
		}
		doc.write_to_file_formatted(filename);
	}
}

void Songs::dumpSongs_internal() const {
	if (m_songlist.empty()) return;
	SongVector s = m_songs;
	std::sort(s.begin(), s.end(), CmpByField(&Song::collateByTitle)); dumpXML(s, "Songlist by song name", m_songlist + "/songs-by-title.xhtml");
	std::sort(s.begin(), s.end(), CmpByField(&Song::collateByArtist)); dumpXML(s, "Songlist by artist", m_songlist + "/songs-by-artist.xhtml");
}

