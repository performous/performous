#include <audio.h>
#include <xine.h>

CAudio::CAudio()
{
	xine = xine_new();
        xine_init(xine);
        vo_port = xine_open_video_driver ( xine, NULL, XINE_VISUAL_TYPE_NONE, NULL);    /*Create a fake vo_port*/ 
        ao_port = xine_open_audio_driver(xine , "auto", NULL);
        stream = xine_stream_new(xine, ao_port, vo_port);
        event_queue = xine_event_new_queue(stream);
	length = 0;
}

CAudio::~CAudio()
{
        xine_close(stream);
        xine_event_dispose_queue(event_queue);
        xine_dispose(stream);
        stream = NULL;
        xine_close_audio_driver(xine, ao_port);
        xine_close_video_driver(xine, vo_port);   
        xine_exit(xine);
}

void CAudio::loadMusic( char * )
{
        ;
}

void CAudio::playMusic( char * filename )
{
        int pos_stream;
	int pos_time;

        if (isPlaying()) 
            stopMusic();
        
        if (!xine_open(stream, filename) || !xine_play(stream, 0, 0)) {
            printf("could not open %s\n", filename);
        }

	xine_get_pos_length(stream, &pos_stream, &pos_time, &length);

        xine_playing = 1;
}

int CAudio::getLength( void )
{
  return length;
}

bool CAudio::isPlaying( void )
{
        xine_event_t *event; 
        while((event = xine_event_get(event_queue))) {
            switch(event->type) {
                case XINE_EVENT_UI_PLAYBACK_FINISHED:
                    xine_playing = 0;
                    break;
            }
            xine_event_free(event);
        }
        if(xine_playing) {
		return true;
	} else {
		return false;
	}
}

void CAudio::stopMusic( void )
{
        xine_stop(stream);
}

int CAudio::loadSound( char * filename )
{
        sounds.push_back(filename);
        return sounds.size()-1;
}

void CAudio::playSound( int , int id )
{
        if (isPlaying()) 
            stopMusic();
        
        if (!xine_open(stream, sounds[id]) || !xine_play(stream, 0, 0)) {
            printf("could not open %s\n", sounds[id]);
        }
        xine_playing = 1;
}

int CAudio::getPosition( void )
{
  int pos_stream;
  int length_time;
  int position;
  int pos_time;

  if (xine_get_pos_length(stream, &pos_stream, &pos_time, &length_time)) {
    position = pos_time;
  } else {
    position = 0;
  }

  return position;
}
