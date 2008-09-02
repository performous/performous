#include "audio.hh"
#include "xtime.hh"

#include <iostream>
#include <cmath>

#if defined(__FREEBSD__) || defined(__MACOSX__) 
#  define       get_current_dir_name()  getcwd(NULL, PATH_MAX)
#endif

#define LENGTH_ERROR -1

CAudio::CAudio(std::string const& pdev):
#ifdef USE_FFMPEG_AUDIO
	m_type(),
	m_rs(da::settings(pdev)
	.set_callback(boost::ref(*this))
	.set_channels(2)
	.set_rate(48000)
	.set_debug(std::cerr)),
	m_playback(m_rs) {
#else
	m_type() {
	(void)pdev; // Disable warning about unused argument
#endif
#ifdef USE_FFMPEG_AUDIO
	m_mpeg.reset();
	ffmpeg_paused = false;
#endif
#ifdef USE_LIBXINE_AUDIO
	xine = xine_new();
	xine_init(xine);
	vo_port = xine_open_video_driver (xine, NULL, XINE_VISUAL_TYPE_NONE, NULL);    /*Create a fake vo_port*/ 
	ao_port = xine_open_audio_driver(xine , "auto", NULL);
	stream = xine_stream_new(xine, ao_port, vo_port);
	event_queue = xine_event_new_queue(stream);
	xine_playing = false;
	std::cout << ">>> Using playback device Xine" << std::endl;
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
	std::cout << ">>> Using playback device Gstreamer" << std::endl;
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
#ifdef USE_FFMPEG_AUDIO
	m_mpeg.reset();
#endif
}

#ifdef USE_FFMPEG_AUDIO
void CAudio::operator()(da::pcm_data& areas, da::settings const&) {
	boost::mutex::scoped_lock l(m_mutex);
	std::size_t samples = areas.channels * areas.frames;
	unsigned int size = 0;
	if (m_mpeg && !ffmpeg_paused) {
		std::vector<int16_t> buf;
		m_mpeg->audioQueue.tryPop(buf, samples);
		std::transform(buf.begin(), buf.end(), areas.m_buf, da::conv_from_s16);
		size = buf.size();
		if (size < samples && !m_mpeg->audioQueue.eof() && m_mpeg->position() > 1.0) std::cerr << "Warning: audio decoding too slow (buffer underrun): " << std::endl;
	}
	std::fill(areas.m_buf + size, areas.m_buf + samples, 0.0f);
}
#endif

void CAudio::operator()() {
	for (;;) {
		try {
			Type type;
			std::string filename;
			{
				boost::mutex::scoped_lock l(m_mutex);
				m_ready = true;
				m_condready.notify_all();
				while (m_type == NONE) m_cond.wait(l);
				m_ready = false;
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
		} catch (std::exception& e) {
			std::cerr << "Audio error: " << e.what() << std::endl;
		}
	}
}

unsigned int CAudio::getVolume_internal() {
#ifdef USE_FFMPEG_AUDIO
	return 100;
#endif
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
#ifdef USE_FFMPEG_AUDIO
	(void)_volume;
#endif
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
#ifdef USE_FFMPEG_AUDIO
	length = LENGTH_ERROR;
	m_mpeg.reset(new CFfmpeg(false, true, filename));
	if (m_mpeg->duration() < 0) return;
	length = 1e3 * m_mpeg->duration();
	ffmpeg_paused = false;
#endif
#ifdef USE_LIBXINE_AUDIO
	int pos_stream;
	int pos_time;
	if (!xine_open(stream, filename.c_str()) || !xine_play(stream, 0, 0)) {
		std::cout << "Could not open " << filename << std::endl;
	}
	if (!xine_get_pos_length(stream, &pos_stream, &pos_time, &length)) length = LENGTH_ERROR;
	xine_playing = true;
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
#ifdef USE_FFMPEG_AUDIO
	length = LENGTH_ERROR;
	m_mpeg.reset(new CFfmpeg(false, true, filename));
	m_mpeg->seek(30.);
	if (m_mpeg->duration() < 0) return;
	length = 1e3 * m_mpeg->duration();
	ffmpeg_paused = false;
#endif
#ifdef USE_LIBXINE_AUDIO
	int pos_stream;
	int pos_time;

	if (!xine_open(stream, filename.c_str()) || !xine_play(stream, 0, 0) || !xine_play(stream, 0, 30000)) {
		std::cout << "Could not open " << filename << std::endl;
	}

	if (!xine_get_pos_length(stream, &pos_stream, &pos_time, &length)) length = LENGTH_ERROR;
	xine_playing = true;
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

double CAudio::getLength_internal() {
	if (length != LENGTH_ERROR) return 1e-3 * length;
#ifdef USE_FFMPEG_AUDIO
	return 0.0;
#endif
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
	return length == LENGTH_ERROR ? 0.0 : 1e-3 * length;
}

bool CAudio::isPlaying_internal() {
#ifdef USE_FFMPEG_AUDIO
	return m_mpeg && !m_mpeg->audioQueue.eof();
#endif
#ifdef USE_LIBXINE_AUDIO
	xine_event_t *event; 
	while((event = xine_event_get(event_queue))) {
		if (event->type == XINE_EVENT_UI_PLAYBACK_FINISHED) xine_playing = false;
		xine_event_free(event);
	}
	return xine_playing;
#endif
#ifdef USE_GSTREAMER_AUDIO
	// If the length cannot be computed, we assume that the song is playing
	// (happening in the first fex seconds)
	if (getLength_internal() == 0) return true;
	// If we are not in the last second, then we are not at the end of the song 
	return getLength_internal() - getPosition_internal() > 1.0;
#endif
	return true;
}

void CAudio::stopMusic_internal() {
	// if (!isPlaying_internal()) return;
#ifdef USE_FFMPEG_AUDIO
	m_mpeg.reset();
#endif
#ifdef USE_LIBXINE_AUDIO
	xine_stop(stream);
#endif
#ifdef USE_GSTREAMER_AUDIO
	gst_element_set_state (music, GST_STATE_NULL);
#endif
}

double CAudio::getPosition_internal() {
	double position = 0.0;
#ifdef USE_FFMPEG_AUDIO
	if (m_mpeg) position = m_mpeg->position();
#endif
#ifdef USE_LIBXINE_AUDIO
	int pos_stream;
	int length_time;
	int pos_time;
	position = xine_get_pos_length(stream, &pos_stream, &pos_time, &length_time) ? 1e-3 * pos_time : 0.0;
#endif
#ifdef USE_GSTREAMER_AUDIO
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos;
	position = gst_element_query_position(music, &fmt, &pos) ? double(pos) / GST_SECOND : 0.0;
#endif
	return position;
}

bool CAudio::isPaused_internal() {
#ifdef USE_FFMPEG_AUDIO
	return ffmpeg_paused;
#endif
#ifdef USE_LIBXINE_AUDIO
	return isPlaying_internal() && xine_get_param(stream,XINE_PARAM_SPEED) == XINE_SPEED_PAUSE;
#endif
#ifdef USE_GSTREAMER_AUDIO
	return GST_STATE(music) == GST_STATE_PAUSED;
#endif
}

void CAudio::togglePause_internal() {
	if (!isPlaying_internal()) return;
#ifdef USE_FFMPEG_AUDIO
	ffmpeg_paused = !ffmpeg_paused;
#endif
#ifdef USE_LIBXINE_AUDIO
	xine_set_param(stream, XINE_PARAM_SPEED, isPaused_internal() ? XINE_SPEED_NORMAL : XINE_SPEED_PAUSE);
#endif
#ifdef USE_GSTREAMER_AUDIO
	gst_element_set_state(music, isPaused_internal() ? GST_STATE_PLAYING : GST_STATE_PAUSED);
#endif
}

#ifdef USE_GSTREAMER_AUDIO
#  ifndef GSTREAMER_HAS_SEEK_SIMPLE
#    define gst_element_seek_simple(element,format,flag,pos) (gst_element_seek(element,1.0,format,flag,GST_SEEK_TYPE_SET,pos,GST_SEEK_TYPE_NONE,0))
#    warning "gstreamer < 0.10.7 support is obsolete, please update your gstreamer version"
#  endif
#endif

void CAudio::seek_internal(double seek_dist) {
	if (!isPlaying_internal()) return;
	int position = std::max(0.0, std::min(getLength_internal() - 1.0, getPosition_internal() + seek_dist));
#ifdef USE_FFMPEG_AUDIO
	m_mpeg->seek(position);
#endif
#ifdef USE_LIBXINE_AUDIO
	xine_play(stream, 0, 1e3 * position);
#endif
#ifdef USE_GSTREAMER_AUDIO
	gst_element_set_state(music, GST_STATE_PAUSED);
	GstState state_paused = GST_STATE_PAUSED;
	gst_element_get_state(music, NULL, &state_paused, GST_CLOCK_TIME_NONE);
	if (!gst_element_seek_simple(music, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, position*GST_SECOND))
	  throw std::runtime_error("CAudio::seek_internal() failed");
	gst_element_set_state(music, GST_STATE_PLAYING);
#endif
}


