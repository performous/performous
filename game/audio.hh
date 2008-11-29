#ifndef PERFORMOUS_AUDIO_HH
#define PERFORMOUS_AUDIO_HH

#include <string>

#include "ffmpeg.hh"
#include "notes.hh"
#include <boost/scoped_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <audio.hpp>

/** @short High level audio playback API **/
class Audio {
  public:
	/** Takes libda devstr and sample rate. Throws if the device fails. **/
	Audio(std::string const& pdev, unsigned int rate);
	/** Play a song from the beginning
	 * @param filename the track filename
	 */
	void playMusic(std::string const& filename);
	/** Play/crossfade (TODO) a preview of the song, starting at 30 seconds
	 * @param filename the track filename
	 */
	void playPreview(std::string const& filename);
	bool isPaused() { return m_paused; }
	void stopMusic();
	void fadeout();
	/** Get the length of the currently playing song, in seconds. **/
	double getLength() { return m_length; }
	/**
	 * This methods seek forward in the stream (backwards if
	 * argument is negative), and continues playing.
	 * @param seek_dist number of seconds to seek from current position
	 */
	void seek(double seek_dist);
	/** Is the music playing (loaded and not at EOF yet, pause doesn't matter) **/
	bool isPlaying() const;
	/** Get the current position. If not known or nothing is playing, NaN is returned. **/
	double getPosition() const;
	unsigned int getVolumeMusic() { return m_volumeMusic; }
	unsigned int getVolumePreview() { return m_volumePreview; }
	void setVolumeMusic(unsigned int volume) { setVolume_internal(m_volumeMusic = volume); }
	void setVolumePreview(unsigned int volume) { setVolume_internal(m_volumePreview = volume); }
	void operator()(); // Thread runs here, don't call directly
	void operator()(da::pcm_data& areas, da::settings const&);
	void togglePause() { m_paused = !m_paused; }
	void toggleSynth(Notes const& notes) { m_notes = (m_notes ? NULL : &notes); }
  private:
	boost::recursive_mutex m_mutex;
	size_t m_crossfade; // Position within m_crossbuf
	std::vector<float> m_crossbuf; // Crossfading buffer
	std::string m_filename;
	double m_length;
	double m_volume;
	unsigned m_volumeMusic, m_volumePreview;
	void setVolume_internal(unsigned int volume);
	boost::scoped_ptr<CFfmpeg> m_mpeg;
	bool m_paused;
	bool m_prebuffering;
	Notes const* volatile m_notes;
	da::settings m_rs;
	da::playback m_playback;
};

#endif
