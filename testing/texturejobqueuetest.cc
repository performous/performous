#include "common.hh"

#include "game/texturejobqueue.hh"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// These tests drive TextureJobQueue with a fake loader, so they need neither image files nor OpenGL.
// The fake loader writes the file name into the bitmap's buffer, so an applied bitmap tells which file it came from.

namespace {
	using namespace std::chrono_literals;

	auto constexpr timeout = 5s;  ///< Upper bound for anything the tests wait for. Only reached when a test is failing.

	/// Holds the worker thread inside a load until the test lets it go
	class Gate {
	public:
		/// Called by the loader: report arrival, then wait until opened
		void pass() {
			std::unique_lock<std::mutex> l(m_mutex);
			++m_arrivals;
			m_condition.notify_all();
			m_condition.wait(l, [this] { return m_open; });
		}
		/// Wait until the worker has arrived at the gate the given number of times in total. Returns false on a timeout.
		bool waitForArrivals(unsigned count) {
			std::unique_lock<std::mutex> l(m_mutex);
			return m_condition.wait_for(l, timeout, [&] { return m_arrivals >= count; });
		}
		void open() {
			{
				std::lock_guard<std::mutex> l(m_mutex);
				m_open = true;
			}
			m_condition.notify_all();
		}
	private:
		std::mutex m_mutex;
		std::condition_variable m_condition;
		unsigned m_arrivals = 0;
		bool m_open = false;
	};

	std::string fileOf(Bitmap const& bitmap) {
		return std::string(bitmap.buf.begin(), bitmap.buf.end());
	}

	/// Destroy a queue on another thread, and report whether that finished in time.
	/// On a timeout the queue and its thread are leaked, so that a hang fails this test instead of blocking the whole test run.
	bool destroysInTime(std::unique_ptr<TextureJobQueue> queue) {
		auto done = std::make_shared<std::promise<void>>();
		auto finished = done->get_future();
		std::thread([queue = std::move(queue), done]() mutable {
			queue.reset();
			done->set_value();
		}).detach();
		return finished.wait_for(timeout) == std::future_status::ready;
	}
}

class TextureJobQueueTest : public ::testing::Test {
protected:
	/// The fake loader. Files named "slow*" wait at the gate; files named "missing*" fail, leaving the bitmap empty.
	void load(Bitmap& bitmap, fs::path const& name) {
		auto const file = name.string();
		{
			std::lock_guard<std::mutex> l(m_mutex);
			m_loaded.push_back(file);
			m_loaderThreads.push_back(std::this_thread::get_id());
		}
		if (file.rfind("slow", 0) == 0) gate.pass();
		if (file.rfind("missing", 0) == 0) return;
		bitmap.buf.assign(file.begin(), file.end());
		bitmap.width = static_cast<unsigned>(file.size());
		bitmap.height = 1;
	}

	void TearDown() override {
		gate.open();  // Never leave the worker stuck in a load, even after a failed assertion
		queue.reset();
	}

	/// An apply function that records the file of the bitmap it is given, under the given label
	TextureJobQueue::ApplyFunc record(std::string const& label) {
		return [this, label](Bitmap& bitmap) { applied.push_back(label + ": " + fileOf(bitmap)); };
	}

	/// Call apply() until the condition holds, as the main loop would every frame. Returns false on a timeout.
	bool applyUntil(std::function<bool ()> const& condition) {
		auto const deadline = std::chrono::steady_clock::now() + timeout;
		while (true) {
			queue->apply();
			if (condition()) return true;
			if (std::chrono::steady_clock::now() > deadline) return false;
			std::this_thread::sleep_for(1ms);
		}
	}

	/// Call apply() until the given number of jobs have been applied
	bool applyUntilCount(std::size_t count) {
		return applyUntil([&] { return applied.size() >= count; });
	}

	std::vector<std::string> loaded() {
		std::lock_guard<std::mutex> l(m_mutex);
		return m_loaded;
	}

	std::vector<std::thread::id> loaderThreads() {
		std::lock_guard<std::mutex> l(m_mutex);
		return m_loaderThreads;
	}

	Gate gate;
	std::vector<std::string> applied;  ///< Only touched on the test thread, by apply functions
	std::unique_ptr<TextureJobQueue> queue = std::make_unique<TextureJobQueue>([this](Bitmap& bitmap, fs::path const& name) { load(bitmap, name); });

private:
	std::mutex m_mutex;
	std::vector<std::string> m_loaded;  ///< Every file the loader was asked for, in order
	std::vector<std::thread::id> m_loaderThreads;
};

// Loading and applying

TEST_F(TextureJobQueueTest, aJobIsLoadedAndAppliedWithItsOwnFile) {
	queue->push("cover.png", record("cover"));
	ASSERT_TRUE(applyUntilCount(1));
	EXPECT_THAT(applied, ElementsAre("cover: cover.png"));
	EXPECT_THAT(loaded(), ElementsAre("cover.png"));
}

TEST_F(TextureJobQueueTest, theWholeBitmapReachesTheApplyFunction) {
	unsigned width = 0;
	unsigned height = 0;
	queue->push("cover.png", [&](Bitmap& bitmap) { width = bitmap.width; height = bitmap.height; });
	ASSERT_TRUE(applyUntil([&] { return width != 0; }));
	EXPECT_EQ(std::string("cover.png").size(), width);
	EXPECT_EQ(1u, height);
}

TEST_F(TextureJobQueueTest, loadsRunOnTheWorkerThreadAndApplyRunsOnTheCallingThread) {
	std::thread::id applyThread;
	queue->push("cover.png", [&](Bitmap&) { applyThread = std::this_thread::get_id(); });
	ASSERT_TRUE(applyUntil([&] { return applyThread != std::thread::id(); }));
	EXPECT_EQ(std::this_thread::get_id(), applyThread);
	EXPECT_THAT(loaderThreads(), ElementsAre(::testing::Ne(std::this_thread::get_id())));
}

TEST_F(TextureJobQueueTest, aJobIsNotAppliedBeforeItsLoadFinishes) {
	queue->push("slow.png", record("slow"));
	ASSERT_TRUE(gate.waitForArrivals(1));  // The worker is now inside the load
	for (int i = 0; i < 10; ++i) queue->apply();
	EXPECT_THAT(applied, IsEmpty());
	gate.open();
	ASSERT_TRUE(applyUntilCount(1));
	EXPECT_THAT(applied, ElementsAre("slow: slow.png"));
}

TEST_F(TextureJobQueueTest, aJobIsAppliedOnlyOnce) {
	queue->push("cover.png", record("cover"));
	ASSERT_TRUE(applyUntilCount(1));
	for (int i = 0; i < 10; ++i) queue->apply();
	EXPECT_THAT(applied, ElementsAre("cover: cover.png"));
	EXPECT_THAT(loaded(), ElementsAre("cover.png"));
}

TEST_F(TextureJobQueueTest, aFailedLoadIsStillAppliedWithAnEmptyBitmap) {
	// The job must not stay pending forever: the Texture keeps its placeholder, and the job is forgotten
	bool empty = false;
	queue->push("missing.png", [&](Bitmap& bitmap) { empty = bitmap.buf.empty() && bitmap.width == 0 && bitmap.height == 0; applied.push_back("missing"); });
	ASSERT_TRUE(applyUntilCount(1));
	EXPECT_TRUE(empty);
}

TEST_F(TextureJobQueueTest, applyWithNoJobsDoesNothing) {
	queue->apply();
	EXPECT_THAT(applied, IsEmpty());
	EXPECT_THAT(loaded(), IsEmpty());
}

TEST_F(TextureJobQueueTest, theSameFileCanBeQueuedTwiceAndEachJobGetsItsOwnCopy) {
	queue->push("cover.png", record("first"));
	queue->push("cover.png", record("second"));
	ASSERT_TRUE(applyUntilCount(2));
	EXPECT_THAT(applied, ElementsAre("first: cover.png", "second: cover.png"));
	EXPECT_THAT(loaded(), ElementsAre("cover.png", "cover.png"));
}

TEST_F(TextureJobQueueTest, jobsPushedWhileTheWorkerIsIdleAreAllLoaded) {
	// Each push must wake the worker; a missed notification would leave a job pending forever
	for (int i = 0; i < 20; ++i) {
		auto const file = "cover" + std::to_string(i) + ".png";
		queue->push(file, record(file));
		ASSERT_TRUE(applyUntilCount(static_cast<std::size_t>(i) + 1)) << "job " << i << " was never applied";
	}
	EXPECT_EQ(20u, applied.size());
}

// Ordering

TEST_F(TextureJobQueueTest, jobsAreLoadedAndAppliedOldestFirst) {
	queue->push("slow.png", record("slow"));
	ASSERT_TRUE(gate.waitForArrivals(1));  // Hold the worker, so the rest queue up behind it
	queue->push("c.png", record("c"));
	queue->push("a.png", record("a"));
	queue->push("b.png", record("b"));
	gate.open();
	ASSERT_TRUE(applyUntilCount(4));
	EXPECT_THAT(loaded(), ElementsAre("slow.png", "c.png", "a.png", "b.png"));
	EXPECT_THAT(applied, ElementsAre("slow: slow.png", "c: c.png", "a: a.png", "b: b.png"));
}

// Tickets

TEST_F(TextureJobQueueTest, ticketsAreNonZeroAndIncreasing) {
	TextureJobQueue::Ticket previous = 0;
	for (int i = 0; i < 10; ++i) {
		auto const ticket = queue->push("cover.png", [](Bitmap&) {});
		EXPECT_NE(0u, ticket);
		EXPECT_GT(ticket, previous);
		previous = ticket;
	}
}

TEST_F(TextureJobQueueTest, ticketsAreNotReusedAfterRemoveOrApply) {
	auto const removed = queue->push("removed.png", [](Bitmap&) {});
	queue->remove(removed);
	auto const appliedTicket = queue->push("cover.png", record("cover"));
	ASSERT_TRUE(applyUntilCount(1));
	auto const next = queue->push("next.png", [](Bitmap&) {});
	EXPECT_NE(removed, appliedTicket);
	EXPECT_NE(removed, next);
	EXPECT_NE(appliedTicket, next);
}

// Removing jobs

TEST_F(TextureJobQueueTest, aLoadFinishingAfterItsJobWasRemovedDoesNotReachTheNextJob) {
	// Issue #1142: a Texture is destroyed during its load, and a new Texture is created, possibly at the same address.
	// The finished load must be thrown away, and the new job must load its own file.
	auto const slow = queue->push("slow.svg", record("slow"));
	ASSERT_TRUE(gate.waitForArrivals(1));  // The worker is now inside the slow load
	queue->remove(slow);  // Its Texture is destroyed...
	queue->push("icon.svg", record("icon"));  // ...and a new one created
	gate.open();
	ASSERT_TRUE(applyUntilCount(1));
	EXPECT_THAT(applied, ElementsAre("icon: icon.svg"));  // Keyed by address, this gave "icon: slow.svg"
	EXPECT_THAT(loaded(), ElementsAre("slow.svg", "icon.svg"));
}

TEST_F(TextureJobQueueTest, aRemovedJobIsNotAppliedEvenIfItsLoadHadFinished) {
	auto const first = queue->push("first.png", record("first"));
	queue->push("slow.png", record("slow"));
	// The worker stores a result before it starts the next job, so "first.png" has finished once "slow.png" has started
	ASSERT_TRUE(gate.waitForArrivals(1));
	queue->remove(first);
	gate.open();
	ASSERT_TRUE(applyUntilCount(1));
	for (int i = 0; i < 10; ++i) queue->apply();
	EXPECT_THAT(applied, ElementsAre("slow: slow.png"));
}

TEST_F(TextureJobQueueTest, aJobRemovedBeforeItsLoadStartsIsNeverLoaded) {
	queue->push("slow.png", record("slow"));
	ASSERT_TRUE(gate.waitForArrivals(1));
	auto const removed = queue->push("removed.png", record("removed"));
	queue->push("kept.png", record("kept"));
	queue->remove(removed);
	gate.open();
	ASSERT_TRUE(applyUntilCount(2));
	EXPECT_THAT(loaded(), ElementsAre("slow.png", "kept.png"));
	EXPECT_THAT(applied, ElementsAre("slow: slow.png", "kept: kept.png"));
}

TEST_F(TextureJobQueueTest, removingAnAppliedOrUnknownTicketIsHarmless) {
	auto const done = queue->push("done.png", record("done"));
	ASSERT_TRUE(applyUntilCount(1));
	queue->remove(done);
	queue->remove(0);
	queue->remove(done + 1000);
	queue->push("after.png", record("after"));
	ASSERT_TRUE(applyUntilCount(2));
	EXPECT_THAT(applied, ElementsAre("done: done.png", "after: after.png"));
}

TEST_F(TextureJobQueueTest, removingAJobTwiceIsHarmless) {
	auto const ticket = queue->push("removed.png", record("removed"));
	queue->remove(ticket);
	queue->remove(ticket);
	queue->push("kept.png", record("kept"));
	ASSERT_TRUE(applyUntilCount(1));
	EXPECT_THAT(applied, ElementsAre("kept: kept.png"));
}

// Shutdown

TEST_F(TextureJobQueueTest, anIdleQueueIsDestroyedPromptly) {
	EXPECT_TRUE(destroysInTime(std::move(queue)));
}

TEST_F(TextureJobQueueTest, aQueueIsDestroyedPromptlyRightAfterConstruction) {
	// Destroys while the worker thread is still starting up or just going to sleep. This is the setting of the lost
	// wake-up fixed in 51ced498, but that race's window is only a few instructions wide and this test does not reliably
	// hit it: what prevents it is that m_quit is set, and checked before waiting, under the lock.
	for (int i = 0; i < 500; ++i) {
		ASSERT_TRUE(destroysInTime(std::make_unique<TextureJobQueue>([](Bitmap&, fs::path const&) {}))) << "hung on attempt " << i;
	}
}

TEST_F(TextureJobQueueTest, aQueueWithPendingJobsIsDestroyedPromptlyWithoutApplyingThem) {
	bool appliedAny = false;
	for (int i = 0; i < 100; ++i) queue->push("cover" + std::to_string(i) + ".png", [&](Bitmap&) { appliedAny = true; });
	EXPECT_TRUE(destroysInTime(std::move(queue)));
	EXPECT_FALSE(appliedAny);
}

TEST_F(TextureJobQueueTest, destructionWaitsForTheLoadInProgress) {
	queue->push("slow.png", record("slow"));
	ASSERT_TRUE(gate.waitForArrivals(1));
	auto const start = std::chrono::steady_clock::now();  // Read before the opener starts, so a stall here can't shorten the measured wait
	std::thread opener([this] {
		std::this_thread::sleep_for(50ms);
		gate.open();
	});
	EXPECT_TRUE(destroysInTime(std::move(queue)));
	opener.join();
	EXPECT_GE(std::chrono::steady_clock::now() - start, 40ms);  // It couldn't return until the gate opened
	EXPECT_THAT(applied, IsEmpty());
}
