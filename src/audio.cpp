#include <audio.h>

#include <iostream>
#include <boost/thread/xtime.hpp>
#include <cmath>

#define LENGTH_ERROR -1

CAudio::CAudio(): m_type() {
#ifdef USE_LIBXINE_AUDIO
	xine = xine_new();
	xine_init(xine);
	vo_port = xine_open_video_driver (xine, NULL, XINE_VISUAL_TYPE_NONE, NULL);    /*Create a fake vo_port*/ 
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
	m_thread.reset(new boost::thread(boost::ref(*this)));
}

CAudio::~CAudio() {
	{
		boost::mutex::scoped_lock l(m_mutex);
		m_type = QUIT;
		m_cond.notify_one();
	}
	m_thread->join();
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

namespace {
	// Boost WTF time format, directly from C...
	boost::xtime& operator+=(boost::xtime& time, double seconds) {
		double nsec = 1e9 * (time.sec + seconds) + time.nsec;
		time.sec = boost::xtime::xtime_sec_t(nsec / 1e9);
		time.nsec = boost::xtime::xtime_nsec_t(std::fmod(nsec, 1e9));
		return time;
	}
	boost::xtime operator+(boost::xtime const& left, double seconds) {
		boost::xtime time = left;
		return time += seconds;
	}
	boost::xtime now() {
		boost::xtime time;
		boost::xtime_get(&time, boost::TIME_UTC);
		return time;
	}
}

void CAudio::operator()() {
	for (;;) {
		Type type;
		std::string filename;
		{
			boost::mutex::scoped_lock l(m_mutex);
			while (m_type == NONE) m_cond.wait(l);
			type = m_type;
			m_type = NONE;
			filename = m_filename;
		}
		switch (type) {
		  case NONE: // Should not get here...
		  case QUIT: return;
		  case STOP: stopMusic_internal(); break;
		  case PREVIEW:
			// Wait a little while before actually starting
			boost::thread::sleep(now() + 0.35);
			{
				boost::mutex::scoped_lock l(m_mutex);
				// Did we receive another event already?
				if (m_type != NONE) continue;
			}
			playPreview_internal(filename);
			break;
		  case PLAY: playMusic_internal(filename); break;
		}
	}
}

unsigned int CAudio::getVolume_internal() {
#ifdef USE_LIBXINE_AUDIO
	return xine_get_param(stream, XINE_PARAM_AUDIO_VOLUME);
#endif
#ifdef USE_GSTREAMER_AUDIO
	gdouble vol;
	g_object_get (music, "volume", &vol, NULL);
	return (unsigned int)(vol*100);
#endif
}

void CAudio::setVolume_internal(unsigned int _volume) {
#ifdef USE_LIBXINE_AUDIO
	xine_set_param(stream, XINE_PARAM_AUDIO_VOLUME, _volume);
#endif
#ifdef USE_GSTREAMER_AUDIO
	gdouble vol = _volume/100.;
	g_object_set (music, "volume", vol, NULL);
#endif
}

void CAudio::playMusic_internal(std::string const& filename) {
	stopMusic_internal();
#ifdef USE_LIBXINE_AUDIO
	int pos_stream;
	int pos_time;
	if (!xine_open(stream, filename.c_str()) || !xine_play(stream, 0, 0)) {
		std::cout << "Could not open " << filename << std::endl;
	}
	if (!xine_get_pos_length(stream, &pos_stream, &pos_time, &length)) length = LENGTH_ERROR;
	xine_playing = 1;
#endif
#ifdef USE_GSTREAMER_AUDIO
	if (filename[0] == '/') g_object_set (G_OBJECT (music), "uri", g_strconcat("file://",filename.c_str(),NULL), NULL);
	else g_object_set(G_OBJECT (music), "uri", g_strconcat("file://",get_current_dir_name(),"/",filename.c_str(),NULL), NULL);
	gst_element_set_state (music, GST_STATE_PLAYING);
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 len;
	length = gst_element_query_duration(music, &fmt, &len) ? int(len/GST_MSECOND) : LENGTH_ERROR;
#endif
}

void CAudio::playPreview_internal(std::string const& filename) {
	unsigned int volume = getVolume_internal();
	setVolume_internal(0);
	stopMusic_internal();
#ifdef USE_LIBXINE_AUDIO
	int pos_stream;
	int pos_time;

	if (!xine_open(stream, filename.c_str()) || !xine_play(stream, 0, 0) || !xine_play(stream, 0, 30000)) {
		std::cout << "Could not open " << filename << std::endl;
	}

	if (!xine_get_pos_length(stream, &pos_stream, &pos_time, &length)) length = LENGTH_ERROR;
	xine_playing = 1;
#endif
#ifdef USE_GSTREAMER_AUDIO
	if (filename[0] == '/') g_object_set (G_OBJECT (music), "uri", g_strconcat("file://",filename.c_str(),NULL), NULL);
	else g_object_set (G_OBJECT (music), "uri", g_strconcat("file://",get_current_dir_name(),"/",filename.c_str(),NULL), NULL);
	gst_element_set_state (music, GST_STATE_PAUSED);
	GstState state_paused = GST_STATE_PAUSED;
	gst_element_get_state (music, NULL, &state_paused, GST_CLOCK_TIME_NONE);
	if (!gst_element_seek(music, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
	  GST_SEEK_TYPE_SET, 30*GST_SECOND, GST_SEEK_TYPE_NONE, 0))
	  g_print("playPreview() seek failed\n");
	gst_element_set_state (music, GST_STATE_PLAYING);
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 len;
	if (!gst_element_query_duration (music, &fmt, &len)) length = LENGTH_ERROR;
	else length = int(len/GST_MSECOND);
#endif
	// Wait a little while before restoring volume to prevent clicks
	boost::thread::sleep(now() + 0.05);
	setVolume_internal(volume);
}

int CAudio::getLength_internal() {
	if (length != LENGTH_ERROR) return length;
#ifdef USE_LIBXINE_AUDIO
	int pos_stream;
	int pos_time;
	if (!xine_get_pos_length(stream, &pos_stream, &pos_time, &length)) length = LENGTH_ERROR;
#endif
#ifdef USE_GSTREAMER_AUDIO
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 len;
	length = gst_element_query_duration (music, &fmt, &len) ? int(len/GST_MSECOND) : LENGTH_ERROR;
#endif
	return length == LENGTH_ERROR ? 0 : length;
}

bool CAudio::isPlaying_internal() {
#ifdef USE_LIBXINE_AUDIO
	xine_event_t *event; 
	while((event = xine_event_get(event_queue))) {
		if (event->type == XINE_EVENT_UI_PLAYBACK_FINISHED) xine_playing = 0;
		xine_event_free(event);
	}
	return xine_playing;
#endif
#ifdef USE_GSTREAMER_AUDIO
	// If the length cannot be computed, we assume that the song is playing
	// (happening in the first fex seconds)
	if (getLength_internal() == 0) return true;
	// If we are not in the last second, then we are not at the end of the song 
	return getLength_internal() - getPosition_internal() > 1000;
#endif
	return true;
}

void CAudio::stopMusic_internal() {
	// if (!isPlaying_internal()) return;
#ifdef USE_LIBXINE_AUDIO
	xine_stop(stream);
#endif
#ifdef USE_GSTREAMER_AUDIO
	gst_element_set_state (music, GST_STATE_NULL);
#endif
}

int CAudio::getPosition_internal() {
	int position = 0;
#ifdef USE_LIBXINE_AUDIO
	int pos_stream;
	int length_time;
	int pos_time;
	position = xine_get_pos_length(stream, &pos_stream, &pos_time, &length_time) ? pos_time : 0;
#endif
#ifdef USE_GSTREAMER_AUDIO
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos;
	position = gst_element_query_position(music, &fmt, &pos) ? int(pos / GST_MSECOND) : 0;
#endif
	return position;
}

bool CAudio::isPaused_internal() {
#ifdef USE_LIBXINE_AUDIO
	return isPlaying_internal() && xine_get_param(stream,XINE_PARAM_SPEED) == XINE_SPEED_PAUSE;
#endif
#ifdef USE_GSTREAMER_AUDIO
	return GST_STATE(music) == GST_STATE_PAUSED;
#endif
}

void CAudio::togglePause_internal() {
	if (!isPlaying_internal()) return;
#ifdef USE_LIBXINE_AUDIO
	xine_set_param(stream, XINE_PARAM_SPEED, isPaused_internal() ? XINE_SPEED_NORMAL : XINE_SPEED_PAUSE);
#endif
#ifdef USE_GSTREAMER_AUDIO
	gst_element_set_state(music, isPaused_internal() ? GST_STATE_PLAYING : GST_STATE_PAUSED);
#endif
}

void CAudio::seek_internal(int seek_dist) {
	if (!isPlaying_internal()) return;
	int position = std::max(0, std::min(getLength_internal() - 1000, getPosition_internal() + seek_dist));
#ifdef USE_LIBXINE_AUDIO
	xine_play(stream, 0, position);
#endif
#ifdef USE_GSTREAMER_AUDIO
	gst_element_set_state(music, GST_STATE_PAUSED);
	GstState state_paused = GST_STATE_PAUSED;
	gst_element_get_state(music, NULL, &state_paused, GST_CLOCK_TIME_NONE);
	if (!gst_element_seek_simple(music, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, position*GST_MSECOND))
	  throw std::runtime_error("CAudio::seek_internal() failed");
	gst_element_set_state(music, GST_STATE_PLAYING);
#endif
}


