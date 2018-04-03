#pragma once

#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <vector>

#include "surface.hh"

namespace cv {
	// Forward declarations
	class VideoCapture;
	class VideoWriter;
}

struct CamFrame {
	int width;
	int height;
	std::vector<boost::uint8_t> data;
};

class Webcam {
  public:
	/// cam_id -1 means pick any device
	Webcam(int cam_id = -1);

	~Webcam();

	/// Thread runs here, don't call directly
	void operator()();

	/// Is good?
	bool is_good() const { return m_capture != 0 && m_running; }
	/// When paused, does not get or render frames
	void pause(bool do_pause = true);
	/// Display frame
	void render();

	Dimensions& dimensions() { return m_surface.dimensions; }
	Dimensions const& dimensions() const { return m_surface.dimensions; }

  private:
	std::unique_ptr<boost::thread> m_thread;
	mutable boost::mutex m_mutex;
	std::unique_ptr<cv::VideoCapture> m_capture;
	std::unique_ptr<cv::VideoWriter> m_writer;
	CamFrame m_frame;
	Surface m_surface;
	bool m_frameAvailable;
	volatile bool m_running;
	volatile bool m_quit;

  public:
	static bool enabled() {
		#ifdef USE_OPENCV
		return true;
		#else
		return false;
		#endif
	}
};
