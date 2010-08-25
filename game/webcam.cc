#include <iostream>
#include <stdexcept>

#include "webcam.hh"
#include "xtime.hh"
#include "fs.hh"

#ifdef USE_OPENCV
#include <cv.h>
#include <highgui.h>

#else
// Dummy classes
namespace cv {
	class VideoCapture {};
	class VideoWriter {};
}
#endif

Webcam::Webcam(int cam_id):
  m_thread(), m_capture(), m_writer(), m_frameAvailable(false), m_running(false), m_quit(false)
  {
	#ifdef USE_OPENCV
	// Initialize the capture device
	m_capture.reset(new cv::VideoCapture(cam_id));
	if (!m_capture->isOpened()) {
		if (cam_id != -1) {
			std::clog << "unkown/unkown: Webcam id " << cam_id << " failed, trying autodetecting...";
			m_capture.reset(new cv::VideoCapture(-1));
		}
		if (!m_capture->isOpened())
			throw std::runtime_error("Could not initialize webcam capturing!");
	}
	// Initialize the video writer
	#ifdef SAVE_WEBCAM_VIDEO
	float fps = m_capture->get(CV_CAP_PROP_FPS);
	int framew = m_capture->get(CV_CAP_PROP_FRAME_WIDTH);
	int frameh = m_capture->get(CV_CAP_PROP_FRAME_HEIGHT);
	int codec = CV_FOURCC('P','I','M','1'); // MPEG-1
	std::string out_file((getHomeDir() / std::string("performous-webcam_out.mpg")).string());
	m_writer.reset(new cv::VideoWriter(out_file.c_str(), codec, fps > 0 ? fps : 30.0f, cvSize(framew,frameh)));
	if (!m_writer->isOpened()) {
		std::cout << "Could not initialize webcam video saving!" << std::endl;
		m_writer.reset();
	}
	#endif
	// Start thread
	m_thread.reset(new boost::thread(boost::ref(*this)));
	#else
	(void)cam_id; // Avoid unused warning
	#endif
}

Webcam::~Webcam() {
	m_quit = true;
	#ifdef USE_OPENCV
	if (m_thread) m_thread->join();
	#endif
}

void Webcam::operator()() {
	#ifdef USE_OPENCV
	m_running = true;
	while (!m_quit) {
		if (m_running) {
			try {
				// Get a new frame
				cv::Mat frame;
				*m_capture >> frame;
				if (m_writer) *m_writer << frame;
				boost::mutex::scoped_lock l(m_mutex);
				// Copy the frame to storage
				m_frame.width = frame.cols;
				m_frame.height = frame.rows;
				m_frame.data.assign(frame.data, frame.data + (m_frame.width * m_frame.height * 3));
				// Notify renderer
				m_frameAvailable = true;
			} catch (std::exception&) { std::cerr << "Error capturing webcam frame!" << std::endl; }
		}
		// Sleep a little, much if the cam isn't active
		boost::thread::sleep(now() + (m_running ? 0.015 : 0.5));
	}
	#endif
}

void Webcam::pause(bool do_pause) {
	#ifdef USE_OPENCV
	boost::mutex::scoped_lock l(m_mutex);
	#endif
	m_running = !do_pause;
	m_frameAvailable = false;
}

void Webcam::render() {
	#ifdef USE_OPENCV
	if (!m_capture || !m_running) return;
	// Do we have a new frame available?
	if (m_frameAvailable && !m_frame.data.empty()) {
		boost::mutex::scoped_lock l(m_mutex);
		// Load the image
		m_surface.load(m_frame.width, m_frame.height, pix::BGR, &m_frame.data[0]);
		m_frameAvailable = false;
	}
	m_surface.draw(); // Draw
	#endif
}
