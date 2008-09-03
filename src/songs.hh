#ifndef __SONGS_H__
#define __SONGS_H__

#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/xtime.hpp>
#include <deque>
#include <set>
#include <string>
#include <vector>

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
	std::string str() const { return title + "  by  " + artist; }
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

class coverMath {
	public:
		coverMath(){};
		~coverMath(){};
		virtual float getPosition() = 0;
		virtual unsigned int getTarget() = 0;
		virtual void setTarget( unsigned int ) = 0;
};

class coverMathSimple {
	public:
		coverMathSimple(){m_position=0;};
		~coverMathSimple(){};
		double getPosition() const { return m_position; }
		unsigned int getTarget() const { return m_position; }
		void setTarget( unsigned int _target, unsigned int ) { m_position=_target; }
	private:
		unsigned int m_position;
};

class coverMathAdvanced {
  public:
	coverMathAdvanced(): m_target(), m_songs(), m_position(), m_velocity() {
		boost::xtime_get(&m_time, boost::TIME_UTC);
	}
	double getPosition();
	unsigned int getTarget() const { return m_target; };
	void setTarget(unsigned int target, unsigned int songs) {
		boost::xtime_get(&m_time, boost::TIME_UTC);
		m_target = target;
		if (m_songs == songs) return;
		m_songs = songs;
		m_position = target;
	};
  private:
	boost::xtime m_time;
	unsigned int m_target;
	unsigned int m_songs;
	double m_position;
	double m_velocity;
};

class Songs {
	std::set<std::string> m_songdirs;
  public:
	Songs(std::set<std::string> const& songdirs);
	void reload();
	Song& near(double pos);
	Song& operator[](std::size_t pos) { return *m_filtered[pos]; }
	int size() const { return m_filtered.size(); };
	int empty() const { return m_filtered.empty(); };
	void advance(int diff) {
		int _current = (int(math_cover.getTarget()) + diff) % int(m_filtered.size());
		if (_current < 0) _current += m_filtered.size();
		math_cover.setTarget(_current,this->size());
	}
	int currentId() const { return math_cover.getTarget(); }
	double currentPosition() { return math_cover.getPosition(); };
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
	typedef boost::ptr_set<Song> songlist_t;
	songlist_t m_songs;
	typedef std::vector<Song*> filtered_t;
	filtered_t m_filtered;
	//coverMathSimple math_cover;
	coverMathAdvanced math_cover;
	std::string m_filter;
	int m_order;
	void filter_internal();
	void sort_internal();
};

#endif

