#include <iostream>

#include "webcam.hh"

Webcam::Webcam(int cam_id): m_capture(NULL), m_timestamp(0) {
	m_capture = cvCaptureFromCAM(cam_id);
	if (!m_capture) {
		std::cout << "Could not initialize webcam capturing!" << std::endl;
		return;
	}
}


void Webcam::render(double time) {
	if (!m_capture) return;
	IplImage* frame = 0;
	if (time > m_timestamp + 0.333) {
		frame = cvQueryFrame(m_capture);
		if (!frame) return;
		m_surface.load(frame->width, frame->height, pix::RGB, (const unsigned char*)frame->imageData);
		m_timestamp = time;
	}
	m_surface.draw();
}
