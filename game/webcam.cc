#include <iostream>

#include "webcam.hh"
#include "xtime.hh"
#include "fs.hh"

#ifdef USE_OPENCV
#include <cv.h>
#include <highgui.h>

#define SAVE_WEBCAM_VIDEO
#endif

Webcam::Webcam(int cam_id):
  m_thread(), m_capture(NULL), m_writer(NULL), m_frameAvailable(false), m_running(false), m_quit(false)
  {
	#ifdef USE_OPENCV
	// Initialize the capture device
	m_capture = cvCaptureFromCAM(cam_id);
	if (!m_capture) {
		if (cam_id != -1) {
			std::cout << "Webcam id " << cam_id << " failed, trying autodetecting...";
			m_capture = cvCaptureFromCAM(-1);
		}
		if (!m_capture) {
			std::cout << "Could not initialize webcam capturing!" << std::endl;
			return;
		}
	}
	// Initialize the video writer
	#ifdef SAVE_WEBCAM_VIDEO
	float fps = cvGetCaptureProperty(m_capture, CV_CAP_PROP_FPS);
	int framew = cvGetCaptureProperty(m_capture, CV_CAP_PROP_FRAME_WIDTH);
	int frameh = cvGetCaptureProperty(m_capture, CV_CAP_PROP_FRAME_HEIGHT);
	int codec = CV_FOURCC('P','I','M','1'); // MPEG-1
	std::string out_file((getHomeDir() / std::string("performous-webcam_out.mpg")).string());
	m_writer = cvCreateVideoWriter(out_file.c_str(), codec, fps > 0 ? fps : 30.0f, cvSize(framew,frameh));
	if (!m_writer) std::cout << "Could not initialize webcam video saving!" << std::endl;
	#endif
	// Start thread
	m_thread.reset(new boost::thread(boost::ref(*this)));
	#else
	++cam_id; // dummy
	#endif
}

Webcam::~Webcam() {
	m_quit = true;
	#ifdef USE_OPENCV
	if (m_thread) m_thread->join();
	cvReleaseCapture(&m_capture);
	cvReleaseVideoWriter(&m_writer);
	#endif
}

void Webcam::operator()() {
	#ifdef USE_OPENCV
	m_running = true;
	while (!m_quit) {
		IplImage* frame = 0;
		// Try to get a new frame
		if (m_running) frame = cvQueryFrame(m_capture);
		if (frame) {
			if (m_writer) cvWriteFrame(m_writer, frame);
			boost::mutex::scoped_lock l(m_mutex);
			// Copy the frame to storage
			m_frame.width = frame->width;
			m_frame.height = frame->height;
			m_frame.data.assign(frame->imageData, frame->imageData + (m_frame.width * m_frame.height * 3));
			// Notify renderer
			m_frameAvailable = true;
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
