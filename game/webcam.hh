
#include <cv.h>
#include <highgui.h>

#include "glutil.hh"
#include "surface.hh"

class Webcam {
  public:
	Webcam(int cam_id = -1): m_capture(NULL), m_timestamp(0) {
		m_capture = cvCaptureFromCAM(cam_id);
		if (!m_capture) {
			std::cout << "Could not initialize capturing!" << std::endl;
			return;
		}
	}

	~Webcam() { cvReleaseCapture(&m_capture); }

	void render(double time) {
		if (!m_capture) return;
		IplImage* frame = 0;
		if (time > m_timestamp + 0.5) {
			frame = cvQueryFrame(m_capture);
			if (!frame) return;
			m_surface.load(frame->width, frame->height, pix::RGB, (const unsigned char*)frame->imageData);
			m_timestamp = time;
		}
		m_surface.draw();
	}

	Dimensions& dimensions() { return m_surface.dimensions; }
	Dimensions const& dimensions() const { return m_surface.dimensions; }

  private:
	CvCapture* m_capture;
	Surface m_surface;
	double m_timestamp;
};
