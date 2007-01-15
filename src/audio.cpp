#include <audio.h>

#define LENGTH_ERROR -1

#ifdef USE_GSTREAMER
bool gst_playing;
#endif

CAudio::CAudio()
{
#ifdef USE_LIBXINE
	xine = xine_new();
        xine_init(xine);
        vo_port = xine_open_video_driver ( xine, NULL, XINE_VISUAL_TYPE_NONE, NULL);    /*Create a fake vo_port*/ 
        ao_port = xine_open_audio_driver(xine , "auto", NULL);
        stream = xine_stream_new(xine, ao_port, vo_port);
        event_queue = xine_event_new_queue(stream);
#endif
#ifdef USE_GSTREAMER
	/* init GStreamer */
	gst_init (NULL, NULL);
	/* set up */
	music = gst_element_factory_make ("playbin", "play");
	GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (music));
	gst_bus_add_watch ( bus ,bus_call, NULL);
	gst_object_unref (bus);
#endif
	length = 0;
}

CAudio::~CAudio()
{
#ifdef USE_LIBXINE
        xine_close(stream);
        xine_event_dispose_queue(event_queue);
        xine_dispose(stream);
        stream = NULL;
        xine_close_audio_driver(xine, ao_port);
        xine_close_video_driver(xine, vo_port);   
        xine_exit(xine);
#endif
#ifdef USE_GSTREAMER
	gst_object_unref (GST_OBJECT (music));
#endif
}

void CAudio::loadMusic( char * )
{
        ;
}

void CAudio::playMusic( char * filename )
{
        if (isPlaying()) 
            stopMusic();

#ifdef USE_LIBXINE
        int pos_stream;
	int pos_time;

        if (!xine_open(stream, filename) || !xine_play(stream, 0, 0)) {
            printf("could not open %s\n", filename);
        }

	if( !xine_get_pos_length(stream, &pos_stream, &pos_time, &length) )
		length = LENGTH_ERROR;

        xine_playing = 1;
#endif
#ifdef USE_GSTREAMER
	g_object_set (G_OBJECT (music), "uri", g_strconcat("file://",filename,NULL), NULL);
	gst_element_set_state (music, GST_STATE_PLAYING);
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 len;
	if (!gst_element_query_duration (music, &fmt, &len))
		length = LENGTH_ERROR;
	else
		length = (int) (len/1000000);
	gst_playing = 1;
#endif
}

int CAudio::getLength( void )
{
	if( length == LENGTH_ERROR ) {
#ifdef USE_LIBXINE
        	int pos_stream;
		int pos_time;
		if( !xine_get_pos_length(stream, &pos_stream, &pos_time, &length) )
			length = LENGTH_ERROR;
#endif
#ifdef USE_GSTREAMER
		GstFormat fmt = GST_FORMAT_TIME;
		gint64 len;
		if (!gst_element_query_duration (music, &fmt, &len))
			length = LENGTH_ERROR;
		else
			length = (int) (len/1000000);
#endif
	}

	if( length == LENGTH_ERROR )
		return 0;
	else
		return length;
}

bool CAudio::isPlaying( void )
{
#ifdef USE_LIBXINE
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
#endif
#ifdef USE_GSTREAMER
	return gst_playing;
#endif
}

void CAudio::stopMusic( void )
{
#ifdef USE_LIBXINE
        xine_stop(stream);
#endif
#ifdef USE_GSTREAMER
	gst_element_set_state (music, GST_STATE_NULL);
	gst_playing = 0;
#endif
}

int CAudio::loadSound( char * filename )
{
        sounds.push_back(filename);
        return sounds.size()-1;
}

void CAudio::playSound( int , int )
{
}

int CAudio::getPosition( void )
{
  int position;

#ifdef USE_LIBXINE
  int pos_stream;
  int length_time;
  int pos_time;

  if (xine_get_pos_length(stream, &pos_stream, &pos_time, &length_time)) {
    position = pos_time;
  } else {
    position = 0;
  }
#endif
#ifdef USE_GSTREAMER
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 pos;
  if (!gst_element_query_position (music, &fmt, &pos))
    position = 0;
  else
    position = (int) (pos/1000000);
#endif

  return position;
}

#ifdef USE_GSTREAMER
gboolean CAudio::bus_call( GstBus *bus, GstMessage *msg, gpointer    data )
{
	bus = bus;
	data = data;

	switch (GST_MESSAGE_TYPE (msg)) {
		case GST_MESSAGE_EOS:
			gst_playing = 0;
			break;
		case GST_MESSAGE_ERROR: {
			break;
	        }
		case GST_MESSAGE_TAG: {
			break;
		}
		default:
			break;
	}
	return TRUE;
}
#endif
