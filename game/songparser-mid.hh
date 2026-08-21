#pragma once

class Song;

/// MIDI note data is not a standalone song format on its own — it's used as an auxiliary note
/// source by FoF/RockBand-style songs (see IniSongParser) whose header specifies a MIDI file.
namespace SongParserMidi {
	/// Load track names (vocal/instrument) from song.midifilename, for the songs-browser header pass.
	void parseHeader(Song& song);
	/// Load full note data from song.midifilename.
	void parseNotes(Song& song);
}
