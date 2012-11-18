#include "songparser.hh"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <stdexcept>

/// @file
/// Functions used for parsing the SingStar XML song format

using namespace SongParserUtil;

/// 'Magick' to check if this file looks like correct format
bool SongParser::xmlCheck(std::vector<char> const& data) const {
	static const std::string header = "<?";
	return std::equal(header.begin(), header.end(), data.begin());
}

namespace {
	void testAndAdd(Song& s, std::string const& trackid, std::string const& filename) {
		std::string f = s.path + filename;
		if (boost::filesystem::exists(f)) s.music[trackid] = f;
	}
}

#include <libxml++/libxml++.h>
#include <glibmm/convert.h>

/*
// LibXML2 logging facility
extern "C" void xmlLogger(void* logger, char const* msg, ...) { if (logger) *(std::ostream*)logger << msg; }
void enableXMLLogger(std::ostream& os = std::cerr) { xmlSetGenericErrorFunc(&os, xmlLogger); }
void disableXMLLogger() { xmlSetGenericErrorFunc(NULL, xmlLogger); }
*/

struct SSDom: public xmlpp::DomParser {
	xmlpp::Node::PrefixNsMap nsmap;
	SSDom(std::stringstream const& _ss) {
		load(_ss.str());
	}
	void load(std::string const& buf) {
		set_substitute_entities();
		try {
			/*
			struct DisableLogger {
				DisableLogger() { disableXMLLogger(); }
				~DisableLogger() { enableXMLLogger(); }
			} disabler;
			*/
			parse_memory(buf);
		} catch (...) {
			std::string buf2 = Glib::convert(buf, "UTF-8", "ISO-8859-1"); // Convert to UTF-8
			parse_memory(buf2);
		}
		nsmap["ss"] = get_document()->get_root_node()->get_namespace_uri();
	}
	bool find(xmlpp::Element& elem, std::string xpath, xmlpp::NodeSet& n) {
		if (nsmap["ss"].empty()) boost::erase_all(xpath, "ss:");
		n = elem.find(xpath, nsmap);
		return !n.empty();
	}
	bool find(std::string const& xpath, xmlpp::NodeSet& n) {
		return find(*get_document()->get_root_node(), xpath, n);
	}
	bool getValue(std::string const& xpath, std::string& result) {
		xmlpp::NodeSet n;
		if (!find(xpath, n)) return false;
		result = dynamic_cast<xmlpp::Element&>(*n[0]).get_child_text()->get_content();
		return true;
	}
};

/// Parse header data for Songs screen
void SongParser::xmlParseHeader() {
	Song& s = m_song;
	std::string line;
	// only look for artist and title
	bool artistFound = false;
	bool titleFound = false;
	unsigned int i = 0;
	boost::cmatch match;

	boost::regex artistRegex("^\\s*<!--.*Artist:(.*)-->", boost::regex_constants::icase);
	boost::regex titleRegex("^\\s*<!--.*Title:(.*)-->", boost::regex_constants::icase);

	while (getline(line)) {
		if (regex_match(line.c_str(), match, artistRegex)) {
			s.artist = match[1];
			boost::trim(s.artist);
			if(titleFound) break;
			artistFound = true;
		} else if (regex_match(line.c_str(), match, titleRegex)) {
			s.title = match[1];
			boost::trim(s.title);
			if(artistFound) break;
			titleFound = true;
		}
		if (i > 15) break;
		++i;
	}
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");

	boost::regex coverfile("(cover\\..*)$", boost::regex_constants::icase);
	boost::regex videofile("(video\\..*)$", boost::regex_constants::icase);
	boost::regex musicfile("(music\\..*)$", boost::regex_constants::icase);
	boost::regex vocalsfile("(vocals\\..*)$", boost::regex_constants::icase);

	for (boost::filesystem::directory_iterator dirIt(s.path), dirEnd; dirIt != dirEnd; ++dirIt) {
		boost::filesystem::path p = dirIt->path();
#if BOOST_FILESYSTEM_VERSION < 3
		std::string name = p.leaf(); // File basename (notes.xml)
#else
		std::string name = p.filename().string(); // File basename (notes.xml)
#endif
		if (regex_match(name.c_str(), match, coverfile)) {
			s.cover = name;
		} else if (regex_match(name.c_str(), match, videofile)) {
			s.video = name;
		} else if (regex_match(name.c_str(), match, musicfile)) {
			testAndAdd(s, "background", name);
		} else if (regex_match(name.c_str(), match, vocalsfile)) {
			testAndAdd(s, "vocals", name);
		}
	}
	// debug: uncomment this to test parsing on all your song database
	xmlParse();
}

void addNoteToTrack(Song& song, std::string const& name, const Note& note) {
	VocalTrack& vocal = song.vocalTracks.find(name)->second;
	if(note.type != Note::SLEEP) {
		vocal.noteMin = std::min(vocal.noteMin, note.note);
		vocal.noteMax = std::max(vocal.noteMax, note.note);
	}
	if(note.type == Note::SLEEP && vocal.notes.size() == 0) return; // not adding the first sleep
	vocal.notes.push_back(note);
}

void addNoteToTracks(std::string sentenceSinger, std::string trackSinger, bool multiTrackInOne, std::map<std::string, std::string> singerList, Song& song, const Note& note) {
	if(multiTrackInOne) {
		if(sentenceSinger == "Solo 1" || sentenceSinger == "Solo 2") {
			addNoteToTrack(song, singerList[sentenceSinger], note);  // Add to a solo track
			addNoteToTrack(song, trackSinger, note);  // Add to the combined track
		} else if(sentenceSinger == "Group") {
			addNoteToTrack(song, singerList["Solo 1"], note);
			addNoteToTrack(song, singerList["Solo 2"], note);
			addNoteToTrack(song, trackSinger, note);
		} else {
			std::cout << "Unknown singer: " << sentenceSinger << std::endl;
		}
	} else {
		addNoteToTrack(song, trackSinger, note);
	}
}

/// Parse notes
void SongParser::xmlParse() {
	// parse content
	SSDom dom(m_ss);
	Song& s = m_song;
	{
		// extract tempo
		xmlpp::NodeSet n;
		dom.find("/ss:MELODY", n) || dom.find("/MELODY", n);
		if (n.empty()) throw std::runtime_error("Unable to find BPM info");
		xmlpp::Element& e = dynamic_cast<xmlpp::Element&>(*n[0]);
		std::string res = e.get_attribute("Resolution")->get_value();
		m_bpm = boost::lexical_cast<double>(e.get_attribute("Tempo")->get_value().c_str());
		if (res == "Semiquaver") {}
		else if (res == "Demisemiquaver") m_bpm *= 2.0;
		else throw std::runtime_error("Unknown tempo resolution: " + res);
	}
	addBPM(0, m_bpm);
	std::map<std::string, std::string> singerList;
	{
		// extract tracks
		xmlpp::NodeSet tracks;
		if (dom.find("/ss:MELODY/ss:TRACK", tracks)) {
			for (xmlpp::NodeSet::iterator it = tracks.begin(); it != tracks.end(); ++it ) {
				xmlpp::Element& trackNode = dynamic_cast<xmlpp::Element&>(**it);
				std::string name = trackNode.get_attribute("Name")->get_value();
				std::string artist = name;
				if(trackNode.get_attribute("Artist")) artist = trackNode.get_attribute("Artist")->get_value();
				if(name == "Player1") {
					singerList.insert(std::make_pair("Solo 1", artist));
				} else if(name == "Player2") {
					singerList.insert(std::make_pair("Solo 2", artist));
				} else {
					std::cout << "Unknown track name: " << name << std::endl;
				}
			}
		}
	}
	std::map<std::string, xmlpp::NodeSet> sentencesList;
	bool multiTrackInOne = false;
	{
		xmlpp::NodeSet sentences;
		// extract sentences
		if(dom.find("/ss:MELODY/ss:SENTENCE", sentences)) {
			// all stuff here (watch SENTENCE.Singer ("Solo 1", "Solo 2" or "Group")
			std::string singers = m_song.artist;
			if(singerList.size() > 1) multiTrackInOne = true;
			for(std::map<std::string, std::string>::const_iterator it = singerList.begin() ; it != singerList.end() ; ++it) {
				if(it == singerList.begin()) {
					singers = it->second;
				} else {
					singers += " & " + it->second;
				}
			}
			sentencesList.insert(std::make_pair(singers, sentences));
		} else {
			xmlpp::NodeSet tracks;
			if (!dom.find("/ss:MELODY/ss:TRACK", tracks)) throw std::runtime_error("Unable to find any sentences in melody XML");
			for (xmlpp::NodeSet::iterator it = tracks.begin(); it != tracks.end(); ++it ) {
				xmlpp::Element& trackNode = dynamic_cast<xmlpp::Element&>(**it);
				std::string singer = trackNode.get_attribute("Artist")->get_value();
				dom.find(trackNode, "ss:SENTENCE", sentences);
				sentencesList.insert(std::make_pair(singer, sentences));
			}
		}
	}

	if(multiTrackInOne) {
		//std::cout << "Duet mode in a single track in " << m_song.path << std::endl;
		// Add group track
		std::string trackSinger = sentencesList.begin()->first;
		VocalTrack vocalBoth(trackSinger);
		m_song.insertVocalTrack(trackSinger, vocalBoth);
		// add each singer track
		for(std::map<std::string, std::string>::const_iterator it = singerList.begin() ; it != singerList.end() ; ++it) {
			trackSinger = it->second;
			VocalTrack vocal(trackSinger);
			m_song.insertVocalTrack(trackSinger, vocal);
		}
	} else {
		for(std::map<std::string, xmlpp::NodeSet>::const_iterator it = sentencesList.begin() ; it != sentencesList.end() ; ++it) {
			std::string trackSinger = it->first;
			VocalTrack vocal(trackSinger);
			m_song.insertVocalTrack(trackSinger, vocal);
		}
	}

	for(std::map<std::string, xmlpp::NodeSet>::const_iterator it0 = sentencesList.begin() ; it0 != sentencesList.end() ; ++it0) {
		// parse sentences
		const xmlpp::NodeSet sentences = it0->second;
		std::string trackSinger = it0->first;
		xmlpp::Node::PrefixNsMap nsmap;
		nsmap["ss"] = "http://www.singstargame.com";
		double ts = 0;
		double sleepts = -1;
		std::string sentenceSinger = (multiTrackInOne ? "Solo 1" : trackSinger);  // Defaults if sentence doesn't specify singer

		for (xmlpp::NodeSet::const_iterator it = sentences.begin(); it != sentences.end(); ++it ) {
			xmlpp::Element& sentenceNode = dynamic_cast<xmlpp::Element&>(**it);
			// fist sentence should not insert a sleep at the beginning
			if (sleepts != -1) sleepts = ts;

			if(sentenceNode.get_attribute("Part") != NULL) {
				std::string section = sentenceNode.get_attribute("Part")->get_value();
				//std::cout << "New section: " << section << ", ts=" << tsTime(ts) << std::endl;
				Song::SongSection tmp(section, tsTime(ts));
				m_song.songsections.push_back(tmp);
			}
			if(multiTrackInOne && sentenceNode.get_attribute("Singer") != NULL) {
				// Singer is only interesting in multiTrackInOne
				sentenceSinger = sentenceNode.get_attribute("Singer")->get_value();
			}
			xmlpp::NodeSet notes = sentenceNode.find("ss:NOTE", nsmap);
			if (notes.empty()) notes = sentenceNode.find("NOTE");
			for (xmlpp::NodeSet::iterator it2 = notes.begin(); it2 != notes.end(); ++it2 ) {
				xmlpp::Element& noteNode = dynamic_cast<xmlpp::Element&>(**it2);
				Note n;

				std::string lyric = noteNode.get_attribute("Lyric")->get_value();
				if (lyric.size() > 0 && lyric[lyric.size() - 1] == '-') {
					if (lyric.size() > 1 && lyric[lyric.size() - 2] == ' ') lyric.erase(lyric.size() - 2);
					else lyric[lyric.size() - 1] = '~';
				} else {
					lyric += ' ';
				}
				unsigned note = boost::lexical_cast<unsigned>(noteNode.get_attribute("MidiNote")->get_value().c_str());
				unsigned duration = boost::lexical_cast<unsigned>(noteNode.get_attribute("Duration")->get_value().c_str());
				if (noteNode.get_attribute("FreeStyle")) n.type = Note::FREESTYLE;
				else if (noteNode.get_attribute("Bonus")) n.type = Note::GOLDEN;
				else n.type = Note::NORMAL;

				if(note) {
					// first note of a sentence is a sleep (for the previous sentence)
					if (sleepts > 0) {
						Note sleep;
						sleep.type = Note::SLEEP;
						sleep.begin = tsTime(sleepts);
						sleep.end = tsTime(sleepts);
						addNoteToTracks(sentenceSinger, trackSinger, multiTrackInOne, singerList, m_song, sleep);
					}
					sleepts = 0;

					n.begin = tsTime(ts);
					n.end = tsTime(ts + duration);
					n.syllable = lyric;
					n.note = note;
					n.notePrev = note;
					//std::cout << "N/F/B" << " - " << n.begin << " - " << n.end << " - " << lyric << std::endl;
					addNoteToTracks(sentenceSinger, trackSinger, multiTrackInOne, singerList, m_song, n);
				}


				ts += duration;
			}
		}
	}
}


