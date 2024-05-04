#include "songs.hh"
#include "configuration.hh"
#include "database.hh"
#include "fs.hh"
#include "i18n.hh"
#include "json.hh"
#include "libxml++-impl.hh"
#include "log.hh"
#include "platform.hh"
#include "profiler.hh"
#include "song.hh"

#include "songorder/artist_song_order.hh"
#include "songorder/creator_song_order.hh"
#include "songorder/edition_song_order.hh"
#include "songorder/file_time_song_order.hh"
#include "songorder/genre_song_order.hh"
#include "songorder/language_song_order.hh"
#include "songorder/most_sung_song_order.hh"
#include "songorder/name_song_order.hh"
#include "songorder/path_song_order.hh"
#include "songorder/random_song_order.hh"
#include "songorder/score_song_order.hh"

#include <fmt/format.h>
#include <unicode/stsearch.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <regex>
#include <stdexcept>

namespace {
	void initializeSongOrders(Songs& songs) {
		songs.addSongOrder(std::make_shared<RandomSongOrder>());
		songs.addSongOrder(std::make_shared<NameSongOrder>());
		songs.addSongOrder(std::make_shared<ArtistSongOrder>());
		songs.addSongOrder(std::make_shared<EditionSongOrder>());
		songs.addSongOrder(std::make_shared<GenreSongOrder>());
		songs.addSongOrder(std::make_shared<PathSongOrder>());
		songs.addSongOrder(std::make_shared<LanguageSongOrder>());
		songs.addSongOrder(std::make_shared<ScoreSongOrder>());
		songs.addSongOrder(std::make_shared<MostSungSongOrder>());
		songs.addSongOrder(std::make_shared<FileTimeSongOrder>());
		songs.addSongOrder(std::make_shared<CreatorSongOrder>());
	}
}

Songs::Songs(Database & database, std::string const& songlist)
 : m_songlist(songlist),  m_database(database) {
	m_updateTimer.setTarget(getInf()); // Using this as a simple timer counting seconds

	initializeSongOrders(*this);

	m_order = Cycle<unsigned short>(config["songs/sort-order"].ui(), static_cast<unsigned short>(m_songOrders.size() - 1));

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

const std::string SONGS_CACHE_JSON_FILE = "songs.json";

void Songs::reload_internal() {
	{
		std::unique_lock<std::shared_mutex> l(m_mutex);
		m_songs.clear();
		m_dirty = true;
	}
	std::clog << "songs/notice: Starting to load all songs from cache." << std::endl;

	Profiler prof("songloader");

	auto cache = loadCache();

    prof("load-cache");

	std::clog << "songs/notice: Done loading the cache." << std::endl;
	std::clog << "songs/notice: Starting to load all songs from disk, to update the cache." << std::endl;

	Paths systemSongs = getPathsConfig("paths/system-songs");
	Paths paths = getPathsConfig("paths/songs");
	paths.insert(paths.begin(), systemSongs.begin(), systemSongs.end());

	for (auto it = paths.begin(); m_loading && it != paths.end(); ++it) { //loop through stored directories from config
		try {
			if (!fs::is_directory(*it)) { std::clog << "songs/info: >>> Not scanning: " << *it << " (no such directory)\n"; continue; }
			std::clog << "songs/info: >>> Scanning " << *it << std::endl;
			size_t count = loadedSongs();
			reload_internal(*it, cache);
			size_t diff = loadedSongs() - count;
			if (diff > 0 && m_loading) std::clog << "songs/info: " << diff << " songs loaded\n";
		} catch (std::exception& e) {
			std::clog << "songs/error: >>> Error scanning " << *it << ": " << e.what() << '\n';
		}
	}
	prof("build-list");

	if (m_loading) dumpSongs_internal(); // Dump the songlist to file (if requested)
	std::clog << std::flush;
	m_loading = false;
	std::clog << "songs/notice: Done Loading. Loaded " << loadedSongs() << " Songs." << std::endl;
	CacheSonglist();
	std::clog << "songs/notice: Done Caching." << std::endl;
	doneLoading = true;
	initialize_sort_internal();
}

Songs::Cache Songs::loadCache() {
	const fs::path songsMetaFile = getCacheDir() / SONGS_CACHE_JSON_FILE;
	auto jsonRoot = readJSON(songsMetaFile);
	Cache cache;
	for (auto const& songData : jsonRoot) {
		auto song = std::make_shared<Song> (songData);
		cache[song->filename.string()] = std::move(song);
	}
	return cache;
}

void Songs::CacheSonglist() {
	auto jsonRoot = nlohmann::json::array();
	std::shared_lock<std::shared_mutex> l(m_mutex);
	for (auto const& song : m_songs) {
		auto songObject = nlohmann::json::object();

		songObject["id"] = static_cast<int>(song->id);
		if(!song->path.string().empty()) {
			songObject["txtFileFolder"] = song->path.string();
		}
		if(!song->filename.string().empty()) {
			songObject["txtFile"] = song->filename.string();
		}
		if(!song->title.empty()) {
			songObject["title"] = song->title;
		}
		if(!song->artist.empty()) {
			songObject["artist"] = song->artist;
		}
		if(!song->edition.empty()) {
			songObject["edition"] = song->edition;
		}
		if(!song->language.empty()) {
			songObject["language"] = song->language;
		}
		if(!song->creator.empty()) {
			songObject["creator"] = song->creator;
		}
		if(!song->genre.empty()) {
			songObject["genre"] = song->genre;
		}
		if(!song->cover.string().empty()) {
			songObject["cover"] = song->cover.string();
		}
		if(!song->background.string().empty()) {
			songObject["background"] = song->background.string();
		}
		if(!song->music[TrackName::BGMUSIC].string().empty()) {
			songObject["songFile"] = song->music[TrackName::BGMUSIC].string();
		}
		if(!song->midifilename.string().empty()) {
			songObject["midiFile"] = song->midifilename.string();
		}
		if(!song->video.string().empty()) {
			songObject["videoFile"] = song->video.string();
		}
		if(!std::isnan(song->start)) {
			songObject["start"] = song->start;
		}
		if(!std::isnan(song->videoGap)) {
			songObject["videoGap"] = song->videoGap;
		}
		if(!std::isnan(song->preview_start)) {
			songObject["previewStart"] = song->preview_start;
		}
		if(!song->music[TrackName::VOCAL_LEAD].string().empty()) {
			songObject["vocals"] = song->music[TrackName::VOCAL_LEAD].string();
		}
		if(!song->music[TrackName::VOCAL_BACKING].string().empty()) {
			songObject["vocalsBacking"] = song->music[TrackName::VOCAL_BACKING].string();
		}
		if(!song->music[TrackName::PREVIEW].string().empty()) {
			songObject["preview"] = song->music[TrackName::PREVIEW].string();
		}
		if(!song->music[TrackName::GUITAR].string().empty()) {
			songObject["guitar"] = song->music[TrackName::GUITAR].string();
		}
		if(!song->music[TrackName::BASS].string().empty()) {
			songObject["bass"] = song->music[TrackName::BASS].string();
		}
		if(!song->music[TrackName::DRUMS].string().empty()) {
			songObject["drums"] = song->music[TrackName::DRUMS].string();
		}
		if(!song->music[TrackName::DRUMS_SNARE].string().empty()) {
			songObject["drumsSnare"] = song->music[TrackName::DRUMS_SNARE].string();
		}
		if(!song->music[TrackName::DRUMS_CYMBALS].string().empty()) {
			songObject["drumsCymbals"] = song->music[TrackName::DRUMS_CYMBALS].string();
		}
		if(!song->music[TrackName::DRUMS_TOMS].string().empty()) {
			songObject["drumsToms"] = song->music[TrackName::DRUMS_TOMS].string();
		}
		if(!song->music[TrackName::KEYBOARD].string().empty()) {
			songObject["keyboard"] = song->music[TrackName::KEYBOARD].string();
		}
		if(!song->music[TrackName::GUITAR_COOP].string().empty()) {
			songObject["guitarCoop"] = song->music[TrackName::GUITAR_COOP].string();
		}
		if(!song->music[TrackName::GUITAR_RHYTHM].string().empty()) {
			songObject["guitarRhythm"] = song->music[TrackName::GUITAR_RHYTHM].string();
		}

		if (!song->m_bpms.empty()) {
			songObject["bpm"] = 15 / song->m_bpms.front().step;
		}

		// Cache songtype also.
		if(song->hasVocals()) {
			songObject["vocalTracks"] = song->vocalTracks.size();
		}
		songObject["keyboardTracks"] = song->hasKeyboard();
		songObject["drumTracks"] = song->hasDrums();
		songObject["danceTracks"] = song->hasDance();
		songObject["guitarTracks"] = song->hasGuitars();
		songObject["loadStatus"] = static_cast<int>(song->loadStatus);

		// Collate info
		songObject["collateByTitle"] = song->collateByTitle;
		songObject["collateByTitleOnly"] = song->collateByTitleOnly;
		songObject["collateByArtist"] = song->collateByArtist;
		songObject["collateByArtistOnly"] = song->collateByArtistOnly;

		if(songObject != nlohmann::json::object()) {
			jsonRoot.emplace_back(std::move(songObject));
		}
	}

	fs::path cacheDir = getCacheDir() / SONGS_CACHE_JSON_FILE;
	writeJSON(jsonRoot, cacheDir);
}

void Songs::reload_internal(fs::path const& parent, Cache cache) {
	try {
		std::regex expression(R"((\.txt|^song\.ini|^notes\.xml|\.sm)$)", std::regex_constants::icase);
		auto iterator = fs::recursive_directory_iterator(parent, fs::directory_options::follow_directory_symlink);
		auto maxDepth = iterator.depth() + 10;
		for (const auto &dir : iterator) { //loop through files
			if (!m_loading) return; // early return in case scanning is long and user wants to exit quickly
			if (dir.is_directory()) {
				continue;
			}
			if (iterator.depth() > maxDepth) {
				std::clog << "songs/info: >>> Not scanning: " << parent.string() << " (maximum depth reached, possibly due to cyclic symlinks)\n";
				continue; 
			}
			fs::path p = dir.path();
			if (!regex_search(p.filename().string(), expression)) {
				continue; //if the folder does not contain any of the requested files, ignore it
			}
			try { //found song file, make a new song with it.
				auto song = std::shared_ptr<Song>{};
				auto match = cache.find(p.string());
				if (match != cache.end()) {
					song = match->second;
				} else {
					std::clog << "songs/notice: Found song which was not in the cache: " << p.string() << std::endl;
					song = std::make_shared<Song> (p);
				}
				std::unique_lock<std::shared_mutex> l(m_mutex);

				song->id = m_database.addSong(song);

				m_songs.emplace_back(song);
				m_dirty = true;
			} catch (SongParserException& e) {
				std::clog << e;
			}
		}
	} catch (std::exception const& e) {
		std::clog << "songs/error: Error accessing " << parent << ": " << e.what() << '\n';
	}
}

/// Store currently selected song on construction and restore the selection on destruction
/// Assumes that m_filtered has been modified and finds the old selection by pointer value.
/// Sets up math_cover so that the old selection is restored if possible, otherwise the first song is selected.
class Songs::RestoreSel {
	Songs& m_s;
	std::weak_ptr<Song> m_sel;
  public:
	/// constructor
	RestoreSel(Songs& s): m_s(s), m_sel(s.currentPtr()) {}
	~RestoreSel() {
		std::ptrdiff_t pos = 0;
		if (auto song = m_sel.lock()) {
			SongCollection& f = m_s.m_filtered;
			auto it = std::find(f.begin(), f.end(), song);
			if (it != f.end()) pos = it - f.begin();
		}
		m_s.math_cover.setTarget(pos, static_cast<std::ptrdiff_t>(m_s.size()));
	}
};

void Songs::update() {
	if (m_dirty && m_updateTimer.get() > 0.5) filter_internal(); // Update with newly loaded songs
	// A hack to move to the first song when the song screen is entered the first time
	static bool first = true;
	if (first) { first = false; math_cover.reset(); math_cover.setTarget(0, static_cast<std::ptrdiff_t>(size())); }
}

void Songs::setFilter(std::string const& val) {
	if (m_filter == val) return;
	m_filter = val;
	filter_internal();
}

void Songs::filter_internal() {
	Profiler prof("filter_internal");
	m_updateTimer.setValue(0.0);
	m_dirty = false;
	RestoreSel restore(*this);
	try {
		auto filtered = SongCollection();
		// if filter text is blank and no type filter is set, just display all songs.
		if (m_filter == std::string() && m_type == 0) {
			std::shared_lock<std::shared_mutex> l(m_mutex);
			filtered = m_songs;
		} else {
			auto filter = icu::UnicodeString::fromUTF8(
				UnicodeUtil::convertToUTF8(m_filter)
			);
			icu::ErrorCode icuError;

			std::shared_lock<std::shared_mutex> l(m_mutex);
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
					icu::StringSearch search = icu::StringSearch(filter, icu::UnicodeString::fromUTF8((*it).strFull()), UnicodeUtil::m_searchCollator.get(), nullptr, icuError);
					return (search.first(icuError) != USEARCH_DONE);
					}

		// If we still haven't returned, it must be a type match with an empty search string.
				return true;
			});
		}
		m_filtered.swap(filtered);
	} catch (...) {
		std::shared_lock<std::shared_mutex> l(m_mutex);
		SongCollection(m_songs.begin(), m_songs.end()).swap(m_filtered);  // Invalid regex => copy everything
	}
	prof("filtered");
	sort_internal();
}

// TODO: determine appropriate time and location to call this function
void Songs::updateSongOrders(SongPtr const& song) {
	for (auto order : m_songOrders) {
		order->update(song, m_database);
	}
}

namespace {

	/// A functor that compares songs based on a selected member field of them.
	template<typename Field> class CmpByField {
		Field Song::* m_field;
		bool m_ascending;
	  public:
		/** @param field a pointer to the field to use (pointer to member) **/
		CmpByField(Field Song::* field, bool ascending): m_field(field), m_ascending(ascending) {}
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
		bool m_ascending;
	  public:
		/** @param field a pointer to the field to use (pointer to member) **/
		CmpByField(std::string Song::* field, bool ascending): m_field(field), m_ascending(ascending) {}
		/// Compare left < right
		bool operator()(Song const& left , Song const& right) {
			icu::UnicodeString leftVal = icu::UnicodeString::fromUTF8(left.*m_field);
			icu::UnicodeString rightVal = icu::UnicodeString::fromUTF8(right.*m_field);
			icu::ErrorCode sortError;
			UCollationResult result = UnicodeUtil::m_sortCollator->compare(leftVal, rightVal, sortError);
			if (sortError.isSuccess()) {
				return result == UCOL_LESS ? m_ascending : !m_ascending;
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
	template <typename T> CmpByField<T> customComparator(T Song::*field, bool ascending) { return CmpByField<T>(field, ascending); }

	static const unsigned short types = 7;
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

void Songs::typeChange(SortChange diff) {
	if (diff == SortChange::RESET) m_type = 0;
	else {
		int dir = to_underlying(diff);
		m_type = static_cast<unsigned short>((m_type + dir) % types);
		if (m_type >= types)
			m_type = static_cast<unsigned short>(m_type + types);
	}
	filter_internal();
}

void Songs::typeCycle(unsigned short cat) {
	static const unsigned short categories[types] = { 0, 1, 2, 2, 3, 3, 4 };
	// Find the next matching category
	unsigned short type = 0;
	for (unsigned short t = static_cast<unsigned short>(categories[m_type] == cat ? m_type + 1 : 0); t < types; ++t) {
		if (categories[t] == cat) { type = t; break; }
	}
	m_type = type;
	filter_internal();
}

std::string Songs::getSortDescription() const {
	if(m_order < m_songOrders.size())
		return m_songOrders[m_order]->getDescription();

	throw std::logic_error("Internal error: unknown sort order in Songs::getSortDescription");
}

void Songs::sortChange(Game& game, SortChange diff) {
	switch(diff) {
		case SortChange::BACK:
			m_order.backward();
		break;
		case SortChange::FORWARD:
			m_order.forward();
		break;
		case SortChange::RESET:
			m_order.set(0u);
		break;
	}

	RestoreSel restore(*this);
	config["songs/sort-order"].ui() = m_order;

	initialize_sort_internal();
	sort_internal();
	writeConfig(game, false);
}

void Songs::sortSpecificChange(unsigned short sortOrder, bool descending) {
	if(sortOrder < m_songOrders.size())
		m_order = sortOrder;
	else
		m_order = 0;

	RestoreSel restore(*this);
	config["songs/sort-order"].ui() = m_order;

	initialize_sort_internal();
	sort_internal(descending);
}

void Songs::initialize_sort_internal() {
	// don't initialize if loading is still in progress
	if (!doneLoading) {
		return;
	}

	auto& order = *m_songOrders[m_order];

	order.initialize(m_songs, m_database);
}

void Songs::sort_internal(bool descending) {
	if (m_order >= m_songOrders.size())
		m_order.set(0u);

	Profiler prof("sort_internal");

	auto& order = *m_songOrders[m_order];

	order.prepare(m_filtered, m_database);
	prof("prepare");

	std::stable_sort(m_filtered.begin(), m_filtered.end(),
		[&](SongPtr const& a, SongPtr const& b) { return order(*a, *b); });

	if (descending) {
		std::reverse(m_filtered.begin(), m_filtered.end());
	}
	prof("sort");
}

std::shared_ptr<Song> Songs::currentPtr() const try {
	return m_filtered.at(static_cast<size_t>(math_cover.getTarget()));
} catch (std::out_of_range const& e) { return nullptr; }

Song& Songs::current() try {
	return *m_filtered.at(static_cast<size_t>(math_cover.getTarget()));
} catch (std::out_of_range const& e) { throw std::runtime_error(std::string("songs/error: out-of-bounds access attempt for Songs: ") + e.what()); }

Song const& Songs::current() const try {
	return *m_filtered.at(static_cast<size_t>(math_cover.getTarget()));
} catch (std::out_of_range const& e) { throw std::runtime_error(std::string("songs/error: out-of-bounds access attempt for Songs: ") + e.what()); }

namespace {
	void dumpCover(xmlpp::Element* song, Song const& s, size_t num) {
		try {
			std::string ext = s.cover.extension().string();
			if (exists(s.cover)) {
				std::string coverlink = fmt::format("covers/{:04d}{:1}", num, ".jpg");
				if (fs::is_symlink(coverlink)) fs::remove(coverlink);
				create_symlink(s.cover, coverlink);
				xmlpp::set_first_child_text(xmlpp::add_child_element(song, "cover"), coverlink);
			}
		} catch (std::exception& e) {
			std::cerr << "Songlist error handling cover image: " << e.what() << std::endl;
		}
	}

	template<typename SongCollection>
	void dumpXML(SongCollection const& svec, std::string const& filename) {
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
	SongCollection svec = [&] { std::shared_lock<std::shared_mutex> l(m_mutex); return m_songs; }();
	std::sort(svec.begin(), svec.end(), customComparator(&Song::collateByArtist, true));
	fs::path coverpath = fs::path(m_songlist) / "covers";
	fs::create_directories(coverpath);
	dumpXML(svec, m_songlist + "/songlist.xml");
}

void Songs::addSongOrder(SongOrderPtr order) {
	m_songOrders.emplace_back(order);
}
