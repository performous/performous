#ifndef __SONGS_H__
#define __SONGS_H__

#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <deque>
#include <set>
#include <string>
#include <vector>
#include "animvalue.hh"

class MusicalScale {
	double m_baseFreq;
	static const int m_baseId = 33;
  public:
	MusicalScale(double baseFreq = 440.0): m_baseFreq(baseFreq) {}
	std::string getNoteStr(double freq) const;
	unsigned int getNoteNum(int id) const;
	bool isSharp(int id) const;
	double getNoteFreq(int id) const;
	int getNoteId(double freq) const;
	double getNote(double freq) const;
	double getNoteOffset(double freq) const;
};

struct Note {
	double begin, end;
	mutable double power;
	enum Type { FREESTYLE = 'F', NORMAL = ':', GOLDEN = '*', SLEEP = '-'} type;
	int note;
	std::string syllable;
	double diff(double n) const;
	double maxScore() const;
	double score(double freq, double b, double e) const;
  private:
	double scoreMultiplier(double error) const;
};

class SongParser;

class Song: boost::noncopyable {
  public:
	friend class SongParser;
	Song(std::string const& path, std::string const& filename);
	void reload();
	bool parseField(std::string const& line);
	/** Get formatted song label. **/
	std::string str() const { return title + "  by  " + artist; }
	/** Get full song information (used by the search function). **/
	std::string strFull() const { return title + "\n" + artist + "\n" + genre + "\n" + edition + "\n" + path; }
	typedef std::vector<Note> notes_t;
	notes_t notes;
	int noteMin, noteMax;
	std::string path;
	std::string filename;
	std::vector<std::string> category;
	std::string genre;
	std::string edition;
	std::string title;
	std::string artist;
	std::string text;
	std::string creator;
	std::string mp3;
	std::string cover;
	std::string background;
	std::string video;
	double videoGap;
	double start;
	MusicalScale scale;
	std::vector<double> timePitchGraph;
	std::vector<double> pitchPitchGraph;
	std::vector<double> volumePitchGraph;
	std::vector<bool> drawPitchGraph;
	double m_scoreFactor; // Normalization factor for the scoring system
  private:
};

bool operator<(Song const& l, Song const& r);

class Songs: boost::noncopyable {
	std::set<std::string> m_songdirs;
  public:
	Songs(std::set<std::string> const& songdirs);
	~Songs();
	void update() { if (m_dirty) filter_internal(); }
	void reload();
	Song& near(double pos);
	Song& operator[](std::size_t pos) { return *m_filtered[pos]; }
	int size() const { return m_filtered.size(); };
	int empty() const { return m_filtered.empty(); };
	void advance(int diff) {
		int size = m_filtered.size();
		if (size == 0) return;  // Do nothing if no songs are available
		int _current = size ? (int(math_cover.getTarget()) + diff) % size : 0;
		if (_current < 0) _current += m_filtered.size();
		math_cover.setTarget(_current,this->size());
	}
	int currentId() const { return math_cover.getTarget(); }
	double currentPosition() { return math_cover.getValue(); };
	Song& current() { return *m_filtered[math_cover.getTarget()]; }
	Song const& current() const { return *m_filtered[math_cover.getTarget()]; }
	void setFilter(std::string const& regex);
	std::string sortDesc() const;
	void randomize();
	void random() { if (m_order) randomize(); advance(1); }
	void sortChange(int diff);
	void parseFile(Song& tmp);
	void dump(std::ostream& os, std::string const& sort);
  private:
	class RestoreSel;
	typedef std::vector<boost::shared_ptr<Song> > SongVector;
	SongVector m_songs, m_filtered;
	//coverMathSimple math_cover;
	AnimAcceleration math_cover;
	std::string m_filter;
	int m_order;
	void reload_internal();
	void reload_internal(boost::filesystem::path const& p);
	void filter_internal();
	void sort_internal();
	volatile bool m_dirty;
	volatile bool m_loading;
	boost::scoped_ptr<boost::thread> m_thread;
	boost::mutex m_mutex;
};

#endif

