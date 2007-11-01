#include "audio_dev.hpp"
#include <gst/gst.h>
#include <ostream>
#include <stdint.h>

#include <iostream>

namespace da {
	class gst_record: public record::dev {
		static void c_callback(GstElement*, GstBuffer* buffer, GstPad*, gpointer userdata) {
			gst_record& self = *static_cast<gst_record*>(userdata);
			int16_t const* iptr = reinterpret_cast<int16_t const*>(GST_BUFFER_DATA(buffer));
			try {
				// Warning GST_BUFFER_SIZE returns a size in byte and not in frames
				std::vector<sample_t> buf(GST_BUFFER_SIZE(buffer)/sizeof(int16_t));
				std::transform(iptr, iptr + buf.size(), buf.begin(), conv_from_s16);
				std::size_t channels = self.s.channels();
				pcm_data data(&buf[0], (GST_BUFFER_SIZE(buffer) /sizeof(int16_t)) / channels, channels);
				self.s.callback()(data, self.s);
			} catch (std::exception& e) {
				self.s.debug(std::string("Exception from recording callback: ") + e.what());
			}
		}
		settings s;
		struct init {
			init() {
				GError* e = NULL;
				if (!gst_init_check(NULL, NULL, &e)) {
					std::string msg = std::string("GStreamer could not be initialized: ") + e->message;
					g_error_free(e);
					throw std::runtime_error(msg);
				}
			}
			// gst cannot be deinitialized safely :(   ~init() { gst_deinit(); }
		} initialize;
		GstElement* pipeline;
	  public:
		gst_record(settings& s): s(s), initialize() {
			// FIXME: this code probably has cleanup trouble in case of exceptions

			pipeline = gst_pipeline_new("record-pipeline");

			GstElement* source;
			if (!(source = gst_element_factory_make("alsasrc", "record-source")))
			  if (!(source = gst_element_factory_make("osssrc", "record-source")))
			  if (!(source = gst_element_factory_make("osxaudiosrc", "record-source")))
			  throw std::runtime_error("Cannot create record source");

			GstElement* audioconvert;
			if (!(audioconvert = gst_element_factory_make("audioconvert", NULL)))
			  throw std::runtime_error("Cannot create audioconvert");

			GstElement* audioresample;
			if (!(audioresample = gst_element_factory_make("audioresample", NULL)))
			  throw std::runtime_error("Cannot create audioresample");

			GstElement* sink;
			if (!(sink = gst_element_factory_make("fakesink", "record-sink")))
			  throw std::runtime_error("Cannot create fakesink");
			
			gst_bin_add_many(GST_BIN(pipeline), source, audioconvert, audioresample, sink, NULL);
			g_object_set(G_OBJECT(sink), "sync", TRUE, NULL);
			g_object_set(G_OBJECT(sink), "signal-handoffs", TRUE, NULL);
			g_signal_connect(G_OBJECT(sink), "handoff", G_CALLBACK(c_callback), this);
			
			/* Link the elements together */
			GstCaps* caps = gst_caps_new_simple(
			  "audio/x-raw-int",
			  "rate", G_TYPE_INT, s.rate(),
			  "width", G_TYPE_INT, 16,
			  "depth", G_TYPE_INT, 16,
			  "channels", G_TYPE_INT, s.channels(), NULL);
			
			if (!gst_element_link_many(source, audioconvert, audioresample, NULL))
			  throw std::runtime_error("Cannot link the GStreamer elements together ('src' -> 'audioconvert' -> 'audioresample')");

			if (!gst_element_link_filtered(audioresample, sink, caps))
			  throw std::runtime_error("Cannot link the GStreamer elements together ('audioresample' -> 'fakesink')");

			gst_caps_unref(caps);
			
			/* TODO: gst_element_set_state(_pipeline, GST_STATE_PAUSED); */
			gst_element_set_state(pipeline, GST_STATE_PLAYING);
		}
		~gst_record() {
			if (!pipeline) return;
			gst_element_set_state(pipeline, GST_STATE_NULL);
			gst_object_unref(GST_OBJECT(pipeline));
		}
	};
	namespace {
		record_plugin::reg<gst_record> r(devinfo("gst", "GStreamer PCM capture. Settings are not used."));
	}
}

