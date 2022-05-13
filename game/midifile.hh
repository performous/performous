#pragma once
#include "fs.hh"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#if 0

class MidiEvent {
  public:
	/** Constructs new MidiEvent
	* @param fret Fret(1,2,3,4,5) in which the event is connected.
	* @param evt Tells the type of midievent Noteon or Noteoff.
	* @param time Time, absolute time in seconds. Tells when the event should happen.
	*/
	MidiEvent(int f, std::uint8_t e, double t): fret(f), event(e), time(t) {}

	int fret; ///< Tells the quitar fret where the event is linked.
	std::uint8_t event; ///< MIDI event type
	double time; ///< Time in seconds
};

#endif

class MidiStream;

/**
 * The Parser class, that contains needed information of given midi-file
 */

class MidiFileParser {
public:

	/** Constructor
	 *
	 * Creates a MidiFileParser which contains	information of given midifile.
	 *
	 * @param name Name of midifile, which want to be read
	 */
	MidiFileParser(fs::path const& name);

	struct TempoChange {
		std::uint32_t miditime;
		std::uint32_t value;
		TempoChange(std::uint32_t miditime, std::uint32_t value): miditime(miditime), value(value) {}
	};
	typedef std::vector<TempoChange> TempoChanges;
	TempoChanges tempochanges;
	typedef std::uint8_t Pitch;
	struct Note {
		std::uint32_t begin;
		std::uint32_t end;
		Note(std::uint32_t begin, std::uint32_t end = 0): begin(begin), end(end) {}
	};
	typedef std::vector<Note> Notes;
	typedef std::map<Pitch, Notes> NoteMap;
	struct LyricNote {
		std::string lyric;
		float note;
		std::uint32_t begin;
		std::uint32_t end;
		LyricNote(std::string const& lyric, int note, std::uint32_t begin, std::uint32_t end = 0): lyric(lyric), note(static_cast<float>(note)), begin(begin), end(end) {}
	};
	typedef std::vector<LyricNote> Lyrics;
	struct Track {
		std::string name;
		NoteMap notes;
		Lyrics lyrics;
		Track(std::string const& name = "default"): name(name) {}
	};
	typedef std::vector<Track> Tracks;
	Tracks tracks;
	struct MidiSection {
		std::string name;
		double begin;
		MidiSection(std::string const& name, const double begin): name(name), begin(begin) {}
	};
	typedef std::vector<MidiSection> MidiSections;
	MidiSections midisections; ///< vector of song sections
	std::uint16_t parse_header(MidiStream&);
	Track read_track(MidiStream&);
	void cout_midi_event(std::uint8_t type, std::uint8_t arg1, std::uint8_t arg2, std::uint32_t miditime);
	void process_midi_event(Track& track, std::uint8_t type, std::uint8_t arg1, std::uint8_t arg2, std::uint32_t miditime);
	std::uint64_t get_us(std::uint32_t miditime);
	double get_seconds(std::uint32_t miditime) { return 1e-6 * static_cast<double>(get_us(miditime)); }
	void add_tempo_change(std::uint32_t miditime, std::uint32_t tempo);
	std::uint16_t format;
	typedef std::vector<std::string> CommandEvents;
	CommandEvents cmdevents;

	/** Ticks per beat == number of divisions per every quarter note **/
	std::uint16_t division;
	std::uint32_t ts_last;
private:
	std::string m_lyric;
};
