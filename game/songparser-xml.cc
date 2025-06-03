#include "songparser.hh"

#include "util.hh"
#include "libxml++.hh"
#include <boost/algorithm/string.hpp>
#include <stdexcept>

/// @file
/// Functions used for parsing the SingStar XML song format

using namespace SongParserUtil;

/// 'Magick' to check if this file looks like correct format
bool SongParser::xmlCheck(std::string const& data) const {
	static const std::string header = "<?";
	return std::equal(header.begin(), header.end(), data.begin());
}


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
		/*
		struct DisableLogger {
			DisableLogger() { disableXMLLogger(); }
			~DisableLogger() { enableXMLLogger(); }
		} disabler;
		*/
		parse_memory(buf);
		nsmap["ss"] = get_document()->get_root_node()->get_namespace_uri();
	}
	bool find(xmlpp::Element const& elem, std::string xpath, xmlpp::const_NodeSet& n) {
		if (nsmap["ss"].empty()) boost::erase_all(xpath, "ss:");
		n = elem.find(xpath, nsmap);
		return !n.empty();
	}
	bool find(std::string const& xpath, xmlpp::const_NodeSet& n) {
		return find(*get_document()->get_root_node(), xpath, n);
	}
	bool getValue(std::string const& xpath, std::string& result) {
		xmlpp::const_NodeSet n;
		if (!find(xpath, n)) return false;
		result = xmlpp::get_first_child_text(dynamic_cast<const xmlpp::Element&>(*n[0]))->get_content();
		return true;
	}
};

namespace {
	/// Parse str (from XML comment node) for header/value and store cleaned up value in result.
	bool parseComment(std::string const& str, std::string const& header, std::string& result) {
		if (!boost::starts_with(str, header)) return false;
		result = boost::replace_all_copy(
		  boost::trim_left_copy(str.substr(header.size())),
		  "&amp;", "&");
		return true;
	}
}

/// Parse header data for Songs screen
void SongParser::xmlParseHeader() {
	Song& s = m_song;

	// Parse notes.xml
	SSDom dom(m_ss);
	// Extract artist and title from XML comments
	{
		xmlpp::const_NodeSet comments;
		// Comments inside or before root element
		dom.find("/ss:MELODY/comment()", comments) || dom.find("/ss:MELODY/../comment()", comments);
		for (auto const& node: comments) {
			std::string str = dynamic_cast<xmlpp::CommentNode const&>(*node).get_content();
			trim(str);
			parseComment(str, "Artist:", s.artist) || parseComment(str, "Title:", s.title);
		}
		if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
	}

	// Extract tempo
	{
		xmlpp::const_NodeSet n;
		if (!dom.find("/ss:MELODY", n)) throw std::runtime_error("Unable to find BPM info");
		auto const& e = dynamic_cast<xmlpp::Element const&>(*n[0]);
		std::string res = e.get_attribute("Resolution")->get_value();
		SongParserUtil::assign(m_bpm, e.get_attribute("Tempo")->get_value().c_str());
		if (res == "Semiquaver") {}
		else if (res == "Demisemiquaver") m_bpm *= 2.0f;
		else throw std::runtime_error("Unknown tempo resolution: " + res);
	}
	addBPM(0, m_bpm);

	// Read TRACK elements (singer names), if available
	std::string singers;  // Only used for "Together" track
	xmlpp::const_NodeSet tracks;
	dom.find("/ss:MELODY/ss:TRACK", tracks);
	if (!m_song.vocalTracks.empty()) { m_song.vocalTracks.clear(); }
	for (auto const& elem: tracks) {
		auto const& trackNode = dynamic_cast<xmlpp::Element const&>(*elem);
		std::string name = trackNode.get_attribute("Name")->get_value();  // "Player1" or "Player2"
		auto attr = trackNode.get_attribute("Artist");
		std::string artist = attr ? std::string(attr->get_value()) : name;  // Singer name
		if (attr) singers += (singers.empty() ? "" : " & ") + artist;
		m_song.insertVocalTrack(name, VocalTrack(artist));
	}
	// If no tracks are specified, we can assume that it isn't duet
	if (tracks.empty()) m_song.insertVocalTrack("Player1", VocalTrack(s.artist));
	else if (tracks.size() > 1) m_song.insertVocalTrack("Together", VocalTrack(singers.empty() ? _("Together") : singers));
}

void addNoteToTrack(VocalTrack& vocal, const Note& note) {
	if (note.type == Note::Type::SLEEP) {
		// Skip extra sleep notes
		if (vocal.notes.empty() || vocal.notes.back().type == Note::Type::SLEEP) return;
	} else {
		vocal.noteMin = std::min(vocal.noteMin, note.note);
		vocal.noteMax = std::max(vocal.noteMax, note.note);
	}
	vocal.notes.push_back(note);
}

/// Parse notes
void SongParser::xmlParse() {
	// Parse notes.xml
	SSDom dom(m_ss);
	Song& s = m_song;

	// Parse each track...
	xmlpp::const_NodeSet tracks;
	// First try version 1 (MELODY/SENTENCE), fallback to version 2/4 (MELODY/TRACK/SENTENCE).
	dom.find("/ss:MELODY[ss:SENTENCE]", tracks) || dom.find("/ss:MELODY/ss:TRACK[ss:SENTENCE]", tracks);
	if (tracks.empty()) throw std::runtime_error("No valid tracks or sentences found");
	// Process each vocalTrack separately; use the same XML (version 1) track twice if needed
	unsigned players = clamp<unsigned>(static_cast<unsigned>(s.vocalTracks.size()), 1, 2);  // Don't count "Together" track
	auto vocalIt = s.vocalTracks.begin();
	for (unsigned player = 0; player < players; ++player, ++vocalIt) {
		if (vocalIt == s.vocalTracks.end()) throw std::logic_error("SongParser-xml vocalIt past the end");
		VocalTrack& vocal = vocalIt->second;
		auto const& trackElem = dynamic_cast<xmlpp::Element const&>(*tracks[player % tracks.size()]);
		std::string sentenceSinger = "Solo 1";  // The default value
		xmlpp::const_NodeSet sentences;
		dom.find(trackElem, "ss:SENTENCE", sentences);
		unsigned ts = 0;
		for (auto it = sentences.begin(); it != sentences.end(); ++it ) {
			auto const& sentenceNode = dynamic_cast<xmlpp::Element const&>(**it);
			// Check if SENTENCE has new attributes
			{
				auto attr = sentenceNode.get_attribute("Part");
				if (attr) m_song.songsections.push_back(Song::SongSection(attr->get_value(), tsTime(ts)));
				attr = sentenceNode.get_attribute("Singer");
				if (attr) sentenceSinger = attr->get_value();
			}
			// Begin each sentence with sleep (extras will be purged in addNoteToTrack)
			{
				Note sleep;
				sleep.type = Note::Type::SLEEP;
				sleep.begin = sleep.end = tsTime(ts);
				addNoteToTrack(vocal, sleep);
			}
			// Notes of a sentence
			xmlpp::const_NodeSet notes;
			dom.find(sentenceNode, "ss:NOTE", notes);
			for (auto const& elem: notes) {
				auto const& noteNode = dynamic_cast<xmlpp::Element const&>(*elem);
				Note n = xmlParseNote(noteNode, ts);
				if (n.note == 0.0f) continue;
				// Skip sentences that do not belong to current player
				if (player != 0 && sentenceSinger == "Solo 1") continue;
				if (players > 1 && player != 1 && sentenceSinger == "Solo 2") continue;
				// Actually add the note to current player's track...
				addNoteToTrack(vocal, n);
			}  // notes
		}  // sentences
	} // players (vocal tracks)
}

Note SongParser::xmlParseNote(xmlpp::Element const& noteNode, unsigned& ts) {
	Note n;
	std::string lyric = noteNode.get_attribute("Lyric")->get_value();
	// Pretty print hyphenation
	if (lyric.size() > 0 && lyric[lyric.size() - 1] == '-') {
		if (lyric.size() > 1 && lyric[lyric.size() - 2] == ' ') lyric.erase(lyric.size() - 2);
		else lyric[lyric.size() - 1] = '~';
	} else {
		lyric += ' ';
	}
	int note;
	double duration;
	SongParserUtil::assign(note, noteNode.get_attribute("MidiNote")->get_value().c_str());
	SongParserUtil::assign(duration, noteNode.get_attribute("Duration")->get_value().c_str());
	if (noteNode.get_attribute("FreeStyle")) n.type = Note::Type::FREESTYLE;
	else if (noteNode.get_attribute("Bonus")) n.type = Note::Type::GOLDEN;
	else n.type = Note::Type::NORMAL;

	n.begin = tsTime(ts);
	ts += static_cast<unsigned>(duration);
	n.end = tsTime(ts);
	n.syllable = lyric;
	n.note = static_cast<float>(note);
	n.notePrev = static_cast<float>(note);

	return n;
}
