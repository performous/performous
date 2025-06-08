#include "log.hh"

#include "configuration.hh"
#include "fs.hh"
#include "profiler.hh"
#include "util.hh"

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <fmt/chrono.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>


#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#elif (BOOST_OS_WINDOWS)
#include <errhandlingapi.h>
#include <fcntl.h>
#include <fileapi.h>
#include <io.h>
#include <ProcessEnv.h>
#include <cstdio>

#if defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif

#define pipe(fd) _pipe(fd, 4096, _O_BINARY)

namespace logger_color_codes {
	LoggerColor black = 0;
	LoggerColor red = LOGGER_RED | LOGGER_INTENSITY; // Actually light_red.
	LoggerColor green = LOGGER_GREEN;
	LoggerColor yellow = LOGGER_RED | LOGGER_GREEN | LOGGER_INTENSITY; // Not gonna use it for now.
	LoggerColor blue = LOGGER_BLUE;
	LoggerColor magenta = LOGGER_BLUE | LOGGER_RED | LOGGER_INTENSITY; // Actually light_magenta according to windows, normal magenta is actually purple.
	LoggerColor cyan = LOGGER_BLUE | LOGGER_GREEN | LOGGER_INTENSITY; // Actually light_cyan according to windows, normal cyan is actually light blue.
	LoggerColor white = LOGGER_BLUE | LOGGER_GREEN | LOGGER_RED | LOGGER_INTENSITY;

	LoggerColor yellow_bold = LOGGER_RED | LOGGER_GREEN; // Actually brown, default win console colors are atrocious.
	LoggerColor red_bold = LOGGER_RED; // Actually normal red (which is darker).
	LoggerColor bold_on_red = white | LOGGER_BACKGROUND(LOGGER_RED); 
}
#define logger_colors(color) logger_color_codes::color

#endif

using IOStream = boost::iostreams::stream<boost::iostreams::file_descriptor_sink>;

// Capture stderr spam from other libraries and log it properly

struct StderrGrabber {
	IOStream stream;
	std::streambuf* backup;
	std::future<void> logger;
#if (BOOST_OS_WINDOWS)
	int handle_fd;
#endif
	StderrGrabber(): backup(std::cerr.rdbuf()) {
#if (BOOST_OS_WINDOWS)
	handle_fd = dup(Platform::stderr_fd);
 	SpdLogger::trace(LogSystem::LOGGER, "stderr fileno={}, stdout fileno={}, handle_fd={}", Platform::stderr_fd, fileno(stdout), handle_fd);
	stream.open((HANDLE)_get_osfhandle(handle_fd), boost::iostreams::close_handle);
#else
	stream.open(dup(Platform::stderr_fd), boost::iostreams::close_handle);
#endif
		std::cerr.rdbuf(stream.rdbuf());  // Make std::cerr write to our stream (which connects to normal stderr)
		std::array<int,2> fd;
		if (pipe(fd.data()) == -1) SpdLogger::notice(LogSystem::STDERR, "`pipe` returned an error: {}", std::strerror(errno));
		dup2(fd[1], Platform::stderr_fd);  // Close stderr and replace it with a copy of pipe begin
		close(fd[1]);  // Close the original pipe begin
		SpdLogger::info(LogSystem::STDERR, "Standard error output redirected here.");
		logger = std::async(std::launch::async, [fdpipe = fd[0]] {
			std::string line;
			unsigned count = 0;
			for (char ch; read(fdpipe, &ch, 1) == 1;) {
				line += ch;
				if (ch != '\n') continue;

				if (line.substr(9,4) != "]:::" && line.find(" / ERROR") == std::string::npos) { // Don't forward our own errors.
					SpdLogger::info(LogSystem::STDERR, fmt::format("stderr redirect: {}", line));
					++count;
				}
				line.clear();
			}
			close(fdpipe);  // Close this end of pipe
			if (count > 0) {
				SpdLogger::notice(LogSystem::STDERR, "{} messages redirected to log.", count);
			}
		});
	}
	~StderrGrabber() {
#if (BOOST_OS_WINDOWS)
	dup2(handle_fd, Platform::stderr_fd);
#else
	int handle = stream->handle();
		dup2(handle, Platform::stderr_fd);  // Restore stderr (closes the pipe, terminating the thread)
#endif
		std::cerr.rdbuf(backup);  // Restore original rdbuf (that writes to normal stderr)
	}
};

std::unique_ptr<StderrGrabber> SpdLogger::grabber;
std::unordered_map<LogSystem, LoggerPtr> SpdLogger::builtLoggers;
std::shared_ptr<spdlog::sinks::dist_sink_mt> SpdLogger::m_sink;
std::shared_ptr<spdlog::sinks::basic_file_sink_mt> SpdLogger::m_profilerSink;
std::shared_mutex SpdLogger::m_LoggerRegistryMutex;
LoggerPtr SpdLogger::m_defaultLogger;
LoggerPtr SpdLogger::m_ProfilerLogger;

SpdLogger::SpdLogger (spdlog::level::level_enum const& consoleLevel) {
	spdlog::init_thread_pool(2048, 1);
	
	initializeSinks(consoleLevel);

	for (const auto& system: LogSystem()) {
		if (system == LogSystem::LOGGER) continue;
		if (system == LogSystem::PROFILER) continue;
		constructLogger(system);
	}
	grabber = std::make_unique<StderrGrabber>();
}

LoggerPtr SpdLogger::constructLogger(const LogSystem system) {
	std::unique_lock lock(m_LoggerRegistryMutex);
	auto newLogger = m_defaultLogger->clone(system);
	try {
		spdlog::register_logger(newLogger);
	}
	catch (spdlog::spdlog_ex const& e) {
		m_defaultLogger->log(spdlog::level::trace, "Logger for system {} was already registered.", system);
	}
	builtLoggers.try_emplace(system, newLogger);
	newLogger->log(spdlog::level::trace, fmt::format("Logger subsystem initialized, system: {}", system));
	return newLogger;
}

void SpdLogger::initializeSinks(spdlog::level::level_enum const& consoleLevel) {
	auto const time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
	std::string logHeader(fmt::format(
		"{0:*^80}\n"
		"{1:*^80}\n",
			fmt::format(" {} {} starting, {} ", PACKAGE, VERSION, timeFormat(time, "%Y/%m/%d @ %H:%M:%S")),
			fmt::format(" Logging any events of level {}, or higher. ", spdlog::level::to_string_view(consoleLevel))));
	spdlog::filename_t filename = PathCache::getLogFilename().u8string();
	spdlog::filename_t profilerLogFilename = PathCache::getProfilerLogFilename().u8string();
	m_sink = std::make_shared<spdlog::sinks::dist_sink_mt>();
	
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	stdout_sink->set_color(spdlog::level::critical, logger_colors(bold_on_red)); // Error.
	stdout_sink->set_color(spdlog::level::err, logger_colors(yellow)); // Warning.
	stdout_sink->set_color(spdlog::level::warn, logger_colors(green)); // Notice.
	stdout_sink->set_color(spdlog::level::info, logger_colors(white));
	stdout_sink->set_color(spdlog::level::debug, logger_colors(blue));
	stdout_sink->set_color(spdlog::level::trace, logger_colors(cyan));
	
	stdout_sink->set_pattern(formatString); //Need to set it separately for the header.

	auto stderr_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
	stderr_sink->set_color(spdlog::level::critical, logger_colors(bold_on_red)); // Error.
	stderr_sink->set_color(spdlog::level::err, logger_colors(yellow)); // Warning.
	stderr_sink->set_color(spdlog::level::warn, logger_colors(green)); // Notice.
	stderr_sink->set_color(spdlog::level::info, logger_colors(white));
	stderr_sink->set_color(spdlog::level::debug, logger_colors(blue));
	stderr_sink->set_color(spdlog::level::trace, logger_colors(cyan));
	
	stderr_sink->set_pattern(formatString); //Need to set it separately for the header.

	spdlog::file_event_handlers handlers;
	handlers.after_open = [logHeader](spdlog::filename_t filename, std::FILE *fstream) { writeLogHeader(filename, fstream, logHeader); };

	m_sink->add_sink(stdout_sink);
	m_sink->add_sink(stderr_sink);

	m_defaultLogger = std::make_shared<spdlog::async_logger>(LogSystem{LogSystem::LOGGER}.toString(), m_sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	
	m_defaultLogger->set_level(spdlog::level::trace);
	spdlog::set_default_logger(m_defaultLogger);

	stdout_sink->set_level(consoleLevel); // Set console level before opening file to prevent trace from the file rotation.

	auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, 1024 * 1024 * 3, 5, true, handlers);

	m_profilerSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(profilerLogFilename, true, handlers);

	m_sink->add_sink(file_sink);

	file_sink->set_level(consoleLevel == spdlog::level::trace ? consoleLevel : spdlog::level::debug);

	m_profilerSink->set_level(consoleLevel == spdlog::level::trace ? consoleLevel : spdlog::level::debug);
	stderr_sink->set_level(spdlog::level::critical);

	auto headerLogger = std::make_shared<spdlog::async_logger>(PACKAGE, stdout_sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	headerLogger->log(spdlog::level::warn, logHeader);
	
	m_sink->set_pattern(formatString);

}

void SpdLogger::toggleProfilerLogger() {
	if (!m_ProfilerLogger) {
		m_ProfilerLogger = std::make_shared<spdlog::async_logger>(LogSystem{LogSystem::PROFILER}.toString(), m_profilerSink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
		m_ProfilerLogger->set_level(spdlog::level::trace);
	}
	if (config["graphic/fps"].b() == true && std::find(m_sink->sinks().begin(), m_sink->sinks().end(), m_profilerSink) == m_sink->sinks().end())  {
		m_sink->add_sink(m_profilerSink); // Profiler gets its own log, but we should also include everything else, to help make sense of it.
		m_profilerSink->set_pattern(formatString);
		notice(LogSystem::LOGGER, "Starting profiler... saving log at={}", PathCache::getProfilerLogFilename());
	}
	else {
		m_sink->remove_sink(m_profilerSink);
		notice(LogSystem::LOGGER, "Stopping profiler...");
	}
}

void SpdLogger::writeLogHeader(spdlog::filename_t filename, std::FILE* fd, std::string header) {
	if (fs::path _filename{filename}; fs::exists(_filename) && fs::file_size(_filename) >30) {
		trace(LogSystem::LOGGER, "Not writing header to {}. File is not empty, probably a previous log being rotated.", filename);
		return;
	}
	if (fd == nullptr) {
		error(LogSystem::LOGGER, "Unable to write to logfile at {}, invalid fstream.", filename);
		return;
		}
	header.append("\n");
	int ret = std::fputs(header.c_str(), fd);
	if (ret == EOF) {
		error(LogSystem::LOGGER, "Unable to write to logfile at {}, error: {}", filename, std::strerror(errno));
	}
}

SpdLogger::~SpdLogger() {
	notice(LogSystem::LOGGER, "More details might be available in {}", PathCache::getLogFilename().u8string());
	grabber.reset();
	spdlog::shutdown();
}

LoggerPtr SpdLogger::getLogger(LogSystem::Values const& loggerName) {
	if (loggerName == LogSystem::LOGGER) {
		return m_defaultLogger;
	}
	if (loggerName == LogSystem::PROFILER) {
		return m_ProfilerLogger;
	}
	LoggerPtr ret;
	try {
		std::shared_lock lock(m_LoggerRegistryMutex);
		ret = builtLoggers.at(loggerName);
	} catch (std::out_of_range const&) {
		// logger not found, did we already build it?
		auto ptr = spdlog::get(subsystemToString(loggerName));
		if (ptr) {
			std::unique_lock lock(m_LoggerRegistryMutex);
			builtLoggers.try_emplace(loggerName, ptr);
			ret = builtLoggers.at(loggerName);
		}
		else {
			ret = constructLogger(loggerName);
		}
	}
	if (ret == nullptr) {
		throw std::runtime_error(fmt::format("Couldn't find nor construct logger for subsystem={}", subsystemToString(loggerName)));
	}
	return ret;
}
