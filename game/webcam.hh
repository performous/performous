#pragma once

#ifdef USE_OPENCV
#include <cv.h>
#include <highgui.h>
#else
typedef void CvCapture;
#endif

#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <vector>

#include "surface.hh"

struct CamFrame {
	int width;
	int height;
	std::vector<uint8_t> data;
};

class Webcam {
  public:
	/// cam_id -1 means pick any device
	Webcam(int cam_id = -1);

	~Webcam();

	/// Thread runs here, don't call directly
	void operator()();

	/// Is good?
	bool is_good() { return m_capture != 0 && m_running; }
	/// When paused, does not get or render frames
	void pause(bool do_pause = true);
	/// Display frame
	void render();

	Dimensions& dimensions() { return m_surface.dimensions; }
	Dimensions const& dimensions() const { return m_surface.dimensions; }

  private:
	boost::scoped_ptr<boost::thread> m_thread;
	mutable boost::mutex m_mutex;
	CvCapture* m_capture;
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
