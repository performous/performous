#ifndef __AUDIO_H_
#define __AUDIO_H_

#include <string>

#include "ffmpeg.hh"
#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <audio.hpp>
#include <boost/scoped_ptr.hpp>

/**
 * Audio playback class. This class enables the audio playback using ffmpeg
 */
class CAudio {
  public:
	/**
	 * Constructor
	 * This constructor initialise the API audio drivers
	 */
	CAudio(std::string const& pdev, unsigned int rate);
	/**
	 * Destructor
	 * This destructor close the API audio drivers
	 */
	~CAudio();
	/**
	 * This method start the audio playback. If a track is already playing it is
	 * stopped. It also compute the length variable.
	 * @param filename the track filename
	 */
	void playMusic(std::string const& filename) {
		boost::mutex::scoped_lock l(m_mutex);
		m_filename = filename;
		m_type = PLAY;
		m_cond.notify_one();
	}
	/**
	 * This method starts the audio playback at 30sec from the beginning of the track.
	 * If a track is already playing it is
	 * stopped. It also computes the length variable.
	 * @param filename the track filename
	 */
	void playPreview(std::string const& filename) {
		boost::mutex::scoped_lock l(m_mutex);
		m_filename = filename;
		m_type = PREVIEW;
		m_cond.notify_one();
	}
	/**
	 * This method returns true if music is playing, but paused.
	 * Otherwise it returns false
	 */
	bool isPaused() { return isPaused_internal(); }
	/**
	 * This methods toggles pause. If paused, it starts playing
	 * normally. If it is playing, the music is paused
	 */
	void togglePause() { togglePause_internal(); }
	/**
	 * This method stop the audio playback
	 */
	void stopMusic() {
		boost::mutex::scoped_lock l(m_mutex);
		m_type = STOP;
		m_cond.notify_one();
	}
	/**
	 * This method returns the length of the current track.
	 */
	double getLength() { return getLength_internal(); }
	/**
	 * This methods seek forward in the stream (backwards if
	 * argument is negative), and continues playing.
	 * @param seek_dist number of seconds to seek from current position
	 */
	void seek(double seek_dist) { seek_internal(seek_dist); }
	/**
	 * This method returns whether or not the track is playing
	 */
	bool isPlaying() { return isPlaying_internal(); }
	/**
	 * This method returns the current position into the playing track. If the
	 * position cannot be computed, 0 is returned.
	 */
	double getPosition() { return getPosition_internal(); }
	unsigned int getVolumeMusic() { return m_volumeMusic; }
	unsigned int getVolumePreview() { return m_volumePreview; }
	void setVolumeMusic(unsigned int volume) { setVolume_internal(m_volumeMusic = volume); }
	void setVolumePreview(unsigned int volume) { setVolume_internal(m_volumePreview = volume); }
	void operator()(); // Thread runs here, don't call directly
	void operator()(da::pcm_data& areas, da::settings const&);
	void wait() {
		boost::mutex::scoped_lock l(m_mutex);
		while (!m_ready || m_type != NONE) m_condready.wait(l);
	}
	void toggleSynth() { m_synth = !m_synth; }
  private:
	enum Type { NONE, STOP, PREVIEW, PLAY, QUIT } m_type;
	std::string m_filename;
	boost::mutex m_mutex;
	boost::condition m_cond;
	boost::condition m_condready;
	boost::scoped_ptr<boost::thread> m_thread;
	bool m_ready;
	int length;
	double m_volume;
	unsigned m_volumeMusic, m_volumePreview;
	void playMusic_internal(std::string const& filename);
	void playPreview_internal(std::string const& filename);
	bool isPaused_internal();
	void togglePause_internal();
	void stopMusic_internal();
	double getLength_internal();
	void seek_internal(double seek_dist);
	bool isPlaying_internal();
	double getPosition_internal();
	void setVolume_internal(unsigned int volume);
	boost::scoped_ptr<CFfmpeg> m_mpeg;
	volatile bool m_paused;
	volatile bool m_synth;
	volatile bool m_prebuffering;
	da::settings m_rs;
	da::playback m_playback;
};

#endif
