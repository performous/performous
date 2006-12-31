#ifndef __AUDIO_H_
#define __AUDIO_H_

#include <xine.h>
#include <vector>
       
class CAudio {
	public:
	CAudio();
	~CAudio();
	void loadMusic( char * filename) ;
	void playMusic( char * filename );
	int getLength( void );
	bool isPlaying( void );
	void stopMusic( void );
	int loadSound( char * filename );
	void playSound( int channel , int id );
	int getPosition( void );
        private:
        std::vector <char *> sounds; 
        xine_t               *xine;
        xine_stream_t        *stream;
        xine_video_port_t    *vo_port;
        xine_audio_port_t    *ao_port;
        xine_event_queue_t   *event_queue;
        bool xine_playing;
	int length;
};

#endif
