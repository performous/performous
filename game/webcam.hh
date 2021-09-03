#pragma once

#include "texture.hh"
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace cv {
	// Forward declarations
	class VideoCapture;
	class VideoWriter;
}

struct CamFrame {
	int width = 0;
	int height = 0;
	std::vector<std::uint8_t> data;
};

class Webcam {
  public:
	Webcam(int cam_id = 0);
	~Webcam();

	/// Thread runs here, don't call directly
	void operator()();

	/// Is good?
	bool is_good() const { return m_capture != 0 && m_running; }
	/// When paused, does not get or render frames
	void pause(bool do_pause = true);
	/// Display frame
	void render();

	Dimensions& dimensions() { return m_texture.dimensions; }
	Dimensions const& dimensions() const { return m_texture.dimensions; }

  private:
	std::unique_ptr<std::thread> m_thread;
	mutable std::mutex m_mutex;
	std::unique_ptr<cv::VideoCapture> m_capture;
	std::unique_ptr<cv::VideoWriter> m_writer;
	CamFrame m_frame;
	Texture m_texture;
	bool m_frameAvailable;
	std::atomic<bool> m_running{ false };
	std::atomic<bool> m_quit{ false };
	const int m_autoDetect;

  public:
	static bool enabled() {
		#ifdef USE_OPENCV
		return true;
		#else
		return false;
		#endif
	}
};
