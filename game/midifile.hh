#pragma once
#include "fs.hh"
#include <map>
#include <string>
#include <vector>

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

#if 0

class MidiEvent {
  public:
	/** Constructs new MidiEvent
	* @param fret Fret(1,2,3,4,5) in which the event is connected.
	* @param evt Tells the type of midievent Noteon or Noteoff.
	* @param time Time, absolute time in seconds. Tells when the event should happen.
	*/
	MidiEvent(int f, unsigned char e, double t): fret(f), event(e), time(t) {}

	int fret; ///< Tells the quitar fret where the event is linked.
	unsigned char event; ///< MIDI event type
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
		uint32_t miditime;
		uint32_t value;
		TempoChange(uint32_t miditime, uint32_t value): miditime(miditime), value(value) {}
	};
	typedef std::vector<TempoChange> TempoChanges;
	TempoChanges tempochanges;
	typedef uint8_t Pitch;
	struct Note {
		uint32_t begin;
		uint32_t end;
		Note(uint32_t begin, uint32_t end = 0): begin(begin), end(end) {}
	};
	typedef std::vector<Note> Notes;
	typedef std::map<Pitch, Notes> NoteMap;
	struct LyricNote {
		std::string lyric;
		int note;
		uint32_t begin;
		uint32_t end;
		LyricNote(std::string const& lyric, int note, uint32_t begin, uint32_t end = 0): lyric(lyric), note(note), begin(begin), end(end) {}
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
		float begin;
		MidiSection(std::string const& name, const double begin): name(name), begin(begin) {}
	};
	typedef std::vector<MidiSection> MidiSections;
	MidiSections midisections; ///< vector of song sections
	uint16_t parse_header(MidiStream&);
	Track read_track(MidiStream&);
	void cout_midi_event(uint8_t type, uint8_t arg1, uint8_t arg2, uint32_t miditime);
	void process_midi_event(Track& track, uint8_t type, uint8_t arg1, uint8_t arg2, uint32_t miditime);
	uint64_t get_us(uint32_t miditime);
	float get_seconds(uint32_t miditime) { return 1e-6 * get_us(miditime); }
	void add_tempo_change(uint32_t miditime, uint32_t tempo);
	uint16_t format;
	typedef std::vector<std::string> CommandEvents;
	CommandEvents cmdevents;

	/** Ticks per beat == number of divisions per every quarter note **/
	uint16_t division;
	uint32_t ts_last;
private:
	std::string m_lyric;
};

