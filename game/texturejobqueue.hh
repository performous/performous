#pragma once

#include "fs.hh"
#include "image.hh"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <utility>

/// Loads image files into Bitmaps on a worker thread, and hands each finished Bitmap back on the thread that calls apply().
/// It contains no OpenGL: how a file is loaded and what is done with the result are both passed in.
class TextureJobQueue {
public:
	using Ticket = std::uint64_t;  ///< Identifies one load request; never reused, 0 means none
	using LoadFunc = std::function<void (Bitmap& bitmap, fs::path const& name)>;  ///< Runs on the worker thread; must not throw
	using ApplyFunc = std::function<void (Bitmap& bitmap)>;  ///< Runs on the thread that calls apply()

	/// Start the worker thread
	explicit TextureJobQueue(LoadFunc load): m_load(std::move(load)), m_thread(&TextureJobQueue::run, this) {}
	/// Stop the worker thread, waiting for a load in progress to finish. Pending jobs are dropped unapplied.
	~TextureJobQueue() {
		{
			std::lock_guard<std::mutex> l(m_mutex);  // Under the lock, so the worker can't miss the wake-up
			m_quit = true;
		}
		m_condition.notify_one();
		m_thread.join();
	}
	TextureJobQueue(TextureJobQueue const&) = delete;
	TextureJobQueue& operator=(TextureJobQueue const&) = delete;

	/// Add a new job and return its ticket. A ticket is never reused, so a finished load can only reach the job it was started for.
	Ticket push(fs::path const& name, ApplyFunc apply) {
		std::lock_guard<std::mutex> l(m_mutex);
		Ticket const ticket = ++m_lastTicket;
		m_jobs.emplace(ticket, Job{ name, std::move(apply), Bitmap() });
		m_condition.notify_one();
		return ticket;
	}
	/// Cancel a job (no effect if it has already been applied)
	void remove(Ticket ticket) {
		std::lock_guard<std::mutex> l(m_mutex);
		m_jobs.erase(ticket);
	}
	/// Run the apply function of every completed job on the calling thread, then forget those jobs.
	/// The callbacks run with the mutex held, so a callback must not call push() or remove()
	/// (e.g. by creating or destroying a Texture): that would deadlock.
	void apply() {
		std::lock_guard<std::mutex> l(m_mutex);
		for (auto it = m_jobs.begin(); it != m_jobs.end();) {
			{
				Job& j = it->second;
				if (!j.name.empty()) { ++it; continue; }  // Job incomplete, skip it
				j.apply(j.bitmap);
			}
			m_jobs.erase(it++);
		}
	}

private:
	struct Job {
		fs::path name;  ///< The file to load; cleared once the load has finished
		ApplyFunc apply;
		Bitmap bitmap;  ///< The loaded image, once the load has finished
	};
	using Jobs = std::map<Ticket, Job>;  ///< Jobs keyed by ever-increasing ticket; maintains load-ordering

	/// The loader main loop: poll for image load jobs and load into RAM
	void run() {
		while (!m_quit) {
			Ticket ticket = 0;
			fs::path name;
			{
				// Take the oldest request that is still pending
				std::unique_lock<std::mutex> l(m_mutex);
				for (auto const& [t, job]: m_jobs) {
					if (job.name.empty()) continue;  // Job already done
					ticket = t;
					name = job.name;
					break;
				}
				// If not found, wait for one
				if (!ticket) {
					if (!m_quit) m_condition.wait(l);  // m_quit is re-checked under the lock to avoid a lost wake-up
					continue;
				}
			}
			// Load image file into buffer
			Bitmap bitmap;
			m_load(bitmap, name);
			// Store the result
			std::lock_guard<std::mutex> l(m_mutex);
			auto it = m_jobs.find(ticket);
			if (it == m_jobs.end()) continue;  // The job was removed: its Texture was destroyed during the load
			it->second.name.clear();  // Mark the job completed
			it->second.bitmap.swap(bitmap);  // Store the bitmap (if we got any)
		}
	}

	LoadFunc const m_load;
	std::atomic<bool> m_quit{ false };
	std::mutex m_mutex;
	std::condition_variable m_condition;
	Jobs m_jobs;
	Ticket m_lastTicket = 0;
	std::thread m_thread;  ///< Declared last, so that it starts after every other member is initialised
};
