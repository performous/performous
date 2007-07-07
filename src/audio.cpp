#include <audio.h>

#define LENGTH_ERROR -1

CAudio::CAudio()
{
#ifdef USE_LIBXINE_AUDIO
	xine = xine_new();
        xine_init(xine);
        vo_port = xine_open_video_driver ( xine, NULL, XINE_VISUAL_TYPE_NONE, NULL);    /*Create a fake vo_port*/ 
        ao_port = xine_open_audio_driver(xine , "auto", NULL);
        stream = xine_stream_new(xine, ao_port, vo_port);
        event_queue = xine_event_new_queue(stream);
	xine_playing = 0;
#endif
#ifdef USE_GSTREAMER_AUDIO
	/* init GStreamer */
	gst_init (NULL, NULL);
	/* set up */
	GstElement *sink=NULL;
	GstElement *fakesink=NULL;
	music = gst_element_factory_make ("playbin", "play");
	/*If you don't want play video with gstreamer*/
	fakesink = gst_element_factory_make ("fakesink", "fakesink");
        g_object_set (G_OBJECT (music), "video-sink", fakesink, NULL);
        /*Output sink*/
        sink = gst_element_factory_make ("gconfaudiosink", "audiosink");
        /* if we could create the gconf sink use that, otherwise let playbin decide */
        if (sink != NULL) {
                /* set the profile property on the gconfaudiosink to "music and movies" */
                if (g_object_class_find_property (G_OBJECT_GET_CLASS (sink), "profile"))
                        g_object_set (G_OBJECT (sink), "profile", 1, NULL);

                g_object_set (G_OBJECT (music), "audio-sink", sink, NULL);
        }
#endif
	length = 0;
}

CAudio::~CAudio()
{
#ifdef USE_LIBXINE_AUDIO
        xine_close(stream);
        xine_event_dispose_queue(event_queue);
        xine_dispose(stream);
        stream = NULL;
        xine_close_audio_driver(xine, ao_port);
        xine_close_video_driver(xine, vo_port);   
        xine_exit(xine);
#endif
#ifdef USE_GSTREAMER_AUDIO
	gst_object_unref (GST_OBJECT (music));
#endif
}

unsigned int CAudio::getVolume()
{
#ifdef USE_LIBXINE_AUDIO
	return xine_get_param( stream, XINE_PARAM_AUDIO_VOLUME );
#endif
#ifdef USE_GSTREAMER_AUDIO
	gdouble vol;
	g_object_get (music, "volume", &vol, NULL);
	return (unsigned int)(vol*100);
#endif
}

void CAudio::setVolume( unsigned int _volume )
{
#ifdef USE_LIBXINE_AUDIO
	xine_set_param( stream, XINE_PARAM_AUDIO_VOLUME, _volume );
#endif
#ifdef USE_GSTREAMER_AUDIO
	gdouble vol = _volume/100.;
	g_object_set (music, "volume", vol, NULL);
#endif
}

void CAudio::playMusic( char * filename )
{
        if (isPlaying()) 
            stopMusic();

#ifdef USE_LIBXINE_AUDIO
        int pos_stream;
	int pos_time;

        if (!xine_open(stream, filename) || !xine_play(stream, 0, 0)) {
            printf("could not open %s\n", filename);
        }

	if( !xine_get_pos_length(stream, &pos_stream, &pos_time, &length) )
		length = LENGTH_ERROR;

        xine_playing = 1;
#endif
#ifdef USE_GSTREAMER_AUDIO
	if( filename[0] == '/' )
		g_object_set (G_OBJECT (music), "uri", g_strconcat("file://",filename,NULL), NULL);
	else
		g_object_set (G_OBJECT (music), "uri", g_strconcat("file://",get_current_dir_name(),"/",filename,NULL), NULL);
	gst_element_set_state (music, GST_STATE_PLAYING);
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 len;
	if (!gst_element_query_duration (music, &fmt, &len))
		length = LENGTH_ERROR;
	else
		length = (int) (len/GST_MSECOND);
#endif
}



void CAudio::playPreview( char * filename )
{
        if (isPlaying()) 
            stopMusic();

#ifdef USE_LIBXINE_AUDIO
        int pos_stream;
	int pos_time;

        if (!xine_open(stream, filename) || !xine_play(stream, 0, 0) || !xine_play(stream, 0, 30000)) {
            printf("could not open %s\n", filename);
        }

	if( !xine_get_pos_length(stream, &pos_stream, &pos_time, &length) )
		length = LENGTH_ERROR;

        xine_playing = 1;
#endif
#ifdef USE_GSTREAMER_AUDIO
	if( filename[0] == '/' )
		g_object_set (G_OBJECT (music), "uri", g_strconcat("file://",filename,NULL), NULL);
	else
		g_object_set (G_OBJECT (music), "uri", g_strconcat("file://",get_current_dir_name(),"/",filename,NULL), NULL);
	gst_element_set_state (music, GST_STATE_PAUSED);
	GstState state_paused = GST_STATE_PAUSED;
	gst_element_get_state (music, NULL, &state_paused, GST_CLOCK_TIME_NONE);
	if( !gst_element_seek(music, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
		GST_SEEK_TYPE_SET, 30*GST_SECOND,
		GST_SEEK_TYPE_SET, 60*GST_SECOND))
		g_print("playPreview() seek failed\n");
	gst_element_set_state (music, GST_STATE_PLAYING);
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 len;
	if (!gst_element_query_duration (music, &fmt, &len))
		length = LENGTH_ERROR;
	else
		length = (int) (len/GST_MSECOND);
#endif
}

int CAudio::getLength( void )
{
	if( length == LENGTH_ERROR ) {
#ifdef USE_LIBXINE_AUDIO
        	int pos_stream;
		int pos_time;
		if( !xine_get_pos_length(stream, &pos_stream, &pos_time, &length) )
			length = LENGTH_ERROR;
#endif
#ifdef USE_GSTREAMER_AUDIO
		GstFormat fmt = GST_FORMAT_TIME;
		gint64 len;
		if (!gst_element_query_duration (music, &fmt, &len))
			length = LENGTH_ERROR;
		else
			length = (int) (len/GST_MSECOND);
#endif
	}

	if( length == LENGTH_ERROR )
		return 0;
	else
		return length;
}

bool CAudio::isPlaying( void )
{
#ifdef USE_LIBXINE_AUDIO
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
#ifdef USE_GSTREAMER_AUDIO
	// If the length cannot be computed, we assume that the song is playing
	// (happening in the first fex seconds)
	if( getLength()==0 )
		return true;

	// If we are not in the last second, then we are not at the end of the song 
	if( getLength() - getPosition() > 1000 )
		return true;
	else
		return false;
#endif
	return true;
}

void CAudio::stopMusic( void )
{
#ifdef USE_LIBXINE_AUDIO
        xine_stop(stream);
#endif
#ifdef USE_GSTREAMER_AUDIO
	gst_element_set_state (music, GST_STATE_NULL);
#endif
}

int CAudio::getPosition( void )
{
	int position = 0;

#ifdef USE_LIBXINE_AUDIO
	int pos_stream;
	int length_time;
	int pos_time;

	if (xine_get_pos_length(stream, &pos_stream, &pos_time, &length_time)) {
		position = pos_time;
	} else {
	position = 0;
	}
#endif
#ifdef USE_GSTREAMER_AUDIO
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos;
	if (!gst_element_query_position (music, &fmt, &pos))
		position = 0;
	else
		position = (int) (pos/GST_MSECOND);
#endif

	return position;
}

bool CAudio::isPaused( void ) {
#ifdef USE_LIBXINE_AUDIO
	if (isPlaying()){
		int speed = xine_get_param(stream,XINE_PARAM_SPEED);
		if (speed == XINE_SPEED_PAUSE)
			return true;
		return false;
	}
	else
		return false;
#endif
#ifdef USE_GSTREAMER_AUDIO
	return GST_STATE(music) ==  GST_STATE_PAUSED;
#endif
}

void CAudio::togglePause( void ){
	if (isPlaying()){
#ifdef USE_LIBXINE_AUDIO
		if (isPaused())
			xine_set_param(stream,XINE_PARAM_SPEED,XINE_SPEED_NORMAL);
		else
			xine_set_param(stream,XINE_PARAM_SPEED,XINE_SPEED_PAUSE);
#endif
#ifdef USE_GSTREAMER_AUDIO
		if (isPaused())
			gst_element_set_state(music, GST_STATE_PLAYING);
		else
			gst_element_set_state(music, GST_STATE_PAUSED);
#endif
	}
}
void CAudio::seek( int seek_dist ){
	if (isPlaying()){
		int position = getPosition()+seek_dist;
		if ( position < 0) position = 0;
		if ( position > getLength() - 1000){
			fprintf(stdout,"seeking too far ahead\n");
			return;
		}
		fprintf(stdout,"seeking from %d to %d\n",getPosition(),position);
#ifdef USE_LIBXINE_AUDIO
		xine_play(stream,0,position);
#endif
#ifdef USE_GSTREAMER_AUDIO
		gst_element_set_state (music, GST_STATE_PAUSED);
		GstState state_paused = GST_STATE_PAUSED;
		gst_element_get_state (music, NULL, &state_paused, GST_CLOCK_TIME_NONE);
		if( !gst_element_seek_simple(music, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, position*GST_MSECOND))
			g_print("playPreview() seek failed\n");
		gst_element_set_state (music, GST_STATE_PLAYING);
#endif
	}
}



