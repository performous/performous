
#include <cv.h>
#include <highgui.h>

#include "surface.hh"

class Webcam {
  public:
	Webcam(int cam_id = -1);

	~Webcam() { cvReleaseCapture(&m_capture); }

	/// Is good?
	bool operator()() { return m_capture != 0; }

	void render(double time);

	Dimensions& dimensions() { return m_surface.dimensions; }
	Dimensions const& dimensions() const { return m_surface.dimensions; }

  private:
	CvCapture* m_capture;
	Surface m_surface;
	double m_timestamp;
};
