#ifndef __AUDIO_H_
#define __AUDIO_H_

#include "../config.h"

/**
 * Audio playback class. This class enables the audio playback using several
 * audio APIs (Xine or GStreamer)
 */
class CAudio {
	public:
	/**
	 * Constructor
	 * This constructor initialise the API audio drivers
	 */
	CAudio();
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
	void playMusic( char * filename );
	/**
	 * This method starts the audio playback at 30sec from the beginning of the track.
	 * If a track is already playing it is
	 * stopped. It also computes the length variable.
	 * @param filename the track filename
	 */
	void playPreview( char * filename );
	/**
	 * This method returns true if music is playing, but paused.
	 * Otherwise it returns false
	 */
	bool isPaused( void );
	/**
	 * This methods toggles pause. If paused, it starts playing
	 * normally. If it is playing, the music is paused
	 */
	void togglePause( void );
	/**
	 * This method stop the audio playback
	 */
	void stopMusic( void );
	/**
	 * This method returns the length of the current track stored in the
	 * length varialble. If the length variable hasn't be already computed
	 * (for any reason) it is computed again.
	 */
	int getLength( void );
	/**
	 * This methods seek forward in the stream (backwards if
	 * argument is negative), and continues playing.
	 * @param seek_dist number of milliseconds to seek from current position
	 */
	void seek(int seek_dist);
	/**
	 * This method returns wether or not the track is playing
	 */
	bool isPlaying( void );
	/**
	 * This method returns the current position into the playing track. If the
	 * position cannot be computed, 0 is returned.
	 */
	int getPosition( void );
        private:
	int length;
#ifdef USE_LIBXINE
        xine_t               *xine;
        xine_stream_t        *stream;
        xine_video_port_t    *vo_port;
        xine_audio_port_t    *ao_port;
        xine_event_queue_t   *event_queue;
        bool xine_playing;
#endif
#ifdef USE_GSTREAMER_AUDIO
	GstElement *music;
#endif
};

#endif
