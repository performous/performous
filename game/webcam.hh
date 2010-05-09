#pragma once

#ifdef USE_OPENCV
#include <cv.h>
#include <highgui.h>
#else
typedef void CvCapture;
#endif

#include "surface.hh"

class Webcam {
  public:
	/// cam_id -1 means pick any device
	Webcam(int cam_id = -1);

	~Webcam();

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
