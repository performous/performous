#include "log.hh"

#include "fs.hh"
#include "profiler.hh"
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <fmt/chrono.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <fstream>
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
#include <stdio.h>
#pragma warning(disable : 4996)
#define pipe(fd) _pipe(fd, 4096, _O_BINARY)
#endif

namespace {
#if (!BOOST_OS_WINDOWS)
	constexpr int stderr_fd = STDERR_FILENO;
#else
	int stderr_fd = fileno(stderr);
#endif
}

/** \file
 * \brief The std::clog logger.
 *
 * General message format: <tt>subsystem/level: message</tt>
 *
 * Example: * \code
 * std::clog << "foo/info: Here's an info message from subsystem foo" << std::endl;
 * \endcode
 *
 * Each message may contain newlines and flushing the stream (i.e. by std::endl or std::flush) must be done
 * when and only when the message is complete.
 *
 * Any lower-case subsystem name including hyphens may be used. The levels, in descending order of priority * are as follows:
 *
 * error    A serious and rare message that usually means that a function requested by user cannot be completed.
 * warning  Less critical errors that should still be emitted sparingly (consider using "debug" for repeated warnings).
 * notice   A non-error situation that might still require user attention (the lowest level displayed by default).
 * info     Any information that might be of interest but that does not appear too often and glog the log output.
 * debug    Any information that is flooded so much that it should normally be suppressed.
 *
 * The user may either choose a desired level of messages to emit, or he may choose a specific subsystem (by
 * substring search) to be monitored all the way down to debug level, in which case only errors from any other
 * subsystems will be printed.
 *
 **/

/** \internal
 * Guard to ensure we're atomically printing to cerr.
 * \attention This only guards from multiple clog interleaving, not other console I/O.
 */
std::mutex log_lock;
using IOStream = boost::iostreams::stream<boost::iostreams::file_descriptor_sink>;

// Capture stderr spam from other libraries and log it properly
// Note: std::cerr retains its normal functionality but other means of writing stderr get redirected to std::clog
#include <future>
struct StderrGrabber {
	IOStream stream;
	std::streambuf* backup;
	std::future<void> logger;
	StderrGrabber(): backup(std::cerr.rdbuf()) {
#if (BOOST_OS_WINDOWS)
	HANDLE stderrHandle = (HANDLE)_get_osfhandle(stderr_fd);
// 	GetStdHandle(STD_ERROR_HANDLE);
	if (stderrHandle == INVALID_HANDLE_VALUE) {
		SpdLogger::notice(LogSystem::STDERR, "Failed to get stderr handle, error code={}", GetLastError());
		return;
	}
// 	stderr_handle = _open_osfhandle((intptr_t)stderrHandle, _O_TEXT);
	
	stream.open(dup(stderr_fd), boost::iostreams::close_handle);
#else
	stream.open(dup(stderr_fd), boost::iostreams::close_handle);
#endif
		std::cerr.rdbuf(stream.rdbuf());  // Make std::cerr write to our stream (which connects to normal stderr)
		int fd[2];
		if (pipe(fd) == -1) SpdLogger::notice(LogSystem::STDERR, "`pipe` returned an error: {}", std::strerror(errno));
		dup2(fd[1], stderr_fd);  // Close stderr and replace it with a copy of pipe begin
		close(fd[1]);  // Close the original pipe begin
		SpdLogger::info(LogSystem::STDERR, "Standard error output redirected here.");
		logger = std::async(std::launch::async, [fdpipe = fd[0]] {
			std::string line;
			unsigned count = 0;
			for (char ch; read(fdpipe, &ch, 1) == 1;) {
				line += ch;
				if (ch != '\n') continue;
				SpdLogger::info(LogSystem::STDERR, fmt::format("stderr redirect: {}", line));
				std::clog << "stderr/info: " + line << std::flush;
				line.clear(); ++count;
			}
			close(fdpipe);  // Close this end of pipe
			if (count > 0) {
				SpdLogger::notice(LogSystem::STDERR, "{} messages redirected to log.", count);
				std::clog << "stderr/notice: " << count << " messages logged to stderr/info\n" << std::flush;
			}
		});
	}
	~StderrGrabber() {
#if (BOOST_OS_WINDOWS)
// 	handle = fileno(stream->handle());
#else
	int handle = stream->handle();
		dup2(handle, stderr_fd);  // Restore stderr (closes the pipe, terminating the thread)
#endif
		std::cerr.rdbuf(backup);  // Restore original rdbuf (that writes to normal stderr)
	}
};
// #else
// struct StderrGrabber {};  // Not supported on Windows
// #endif

std::unique_ptr<StderrGrabber> grabber;

/** \internal The implementation of the stream filter that handles the message filtering. **/
class VerboseMessageSink : public boost::iostreams::sink {
  public:
	std::streamsize write(const char* s, std::streamsize n);};

// defining them in main() causes segfault at exit as they apparently got free'd before we're done using them
static boost::iostreams::stream_buffer<VerboseMessageSink> sb; //!< \internal
static VerboseMessageSink vsm; //!< \internal

//! \internal used to store the default/original clog buffer.
static std::streambuf* default_ClogBuf = nullptr;
fs::ofstream file;

std::string target;
int minLevel;

void writeLog(std::string const& msg) {
	std::lock_guard<std::mutex> l(log_lock);
	std::cerr << msg << std::flush;
	file << msg << std::flush;
}

int numeric(std::string const& level) {
	if (level == "debug") return 0;
	if (level == "info") return 1;
	if (level == "notice") return 2;
	if (level == "warning") return 3;
	if (level == "error") return 4;
	return -1;
}

std::streamsize VerboseMessageSink::write(const char* s, std::streamsize n) {
	std::string line(s, static_cast<size_t>(n));  // Note: s is *not* a c-string, thus we must stop after n chars.
	// Parse prefix as subsystem/level:...
	size_t slash = line.find('/');
	size_t colon = line.find(": ", slash);
	if (slash == std::string::npos || colon == std::string::npos) {
		std::string msg = "logger/error: Invalid log prefix on line [[[\n" + line + "]]]\n";
		write(msg.data(), static_cast<std::streamsize>(msg.size()));
		return n;
	}
	std::string subsystem(line, 0, slash);
	std::string level(line, slash + 1, colon - slash - 1);
	int lev = numeric(level);
	if (lev == -1) {
		std::string msg = "logger/error: Invalid level '" + level + "' line [[[\n" + line + "]]]\n";
		write(msg.data(), static_cast<std::streamsize>(msg.size()));
		return n;
	}
	if (lev >= minLevel || (!target.empty() && subsystem.find(target) != std::string::npos)) {
		writeLog(line);
	}
	return n;
}

Logger::Logger(std::string const& level) {
	if (default_ClogBuf) throw std::logic_error("Multiple loggers constructed. There can only be one.");
	if (level.find_first_of(":/_* ") != std::string::npos) throw std::runtime_error("Invalid logging level specified. Specify either a subsystem name (e.g. logger) or a level (debug, info, notice, warning, error).");
	pathBootstrap();  // So that log filename is known...
	std::string msg = "logger/notice: Logging ";
	{
		std::lock_guard<std::mutex> l(log_lock);
		if (level.empty()) {
			minLevel = 2;  // Display all notices, warnings and errors
			msg += "all notices, warnings and errors.";
		} else if (level == "none") {
			minLevel = 100;
			msg += "disabled.";  // No-one will see this, so what's the point? :)
		} else {
			minLevel = numeric(level);
			if (minLevel == -1 /* Not a valid level name */) {
				minLevel = 4;  // Display errors from any subsystem
				target = level;  // All messages from the given subsystem
				msg += "everything from subsystem " + target + " and all errors.";
			} else {
				msg += "any events of " + level + " or higher level.";
			}
		}
		if (minLevel < 100) {
			fs::path name = getLogFilename();
			fs::create_directories(name.parent_path());
			file.open(name);
			msg += " Log file: " + name.string();
		}
		sb.open(vsm);
		default_ClogBuf = std::clog.rdbuf();
		std::clog.rdbuf(&sb);
		atexit(Logger::teardown);
	}
	std::clog << msg << std::endl;
}

Logger::~Logger() {
	std::clog << "core/notice: More details might be available in " << getLogFilename() << ".\n";
	teardown();
}

void Logger::teardown() {
	grabber.reset();
	if (default_ClogBuf) std::clog << "logger/info: Exiting normally." << std::endl;
	std::lock_guard<std::mutex> l(log_lock);
	if (!default_ClogBuf) return;
	std::clog.rdbuf(default_ClogBuf);
	sb.close();
	file.close();
	default_ClogBuf = nullptr;
}

std::unordered_map<LogSystem, loggerPtr> SpdLogger::builtLoggers;
std::shared_ptr<spdlog::sinks::dist_sink_mt> SpdLogger::m_sink;
std::shared_mutex SpdLogger::m_LoggerRegistryMutex;
loggerPtr SpdLogger::m_defaultLogger;

SpdLogger::SpdLogger (spdlog::level::level_enum const& consoleLevel) {
	Profiler prof("spdLogger-init");
	spdlog::init_thread_pool(2048, 1);

	auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::string logHeader(fmt::format(
		"{0:*^80}\n"
		"{1:*^80}\n",
			fmt::format(" {} {} starting, {:%Y/%m/%d @ %H:%M:%S} ", PACKAGE, VERSION, fmt::localtime(time)),
			fmt::format(" Logging any events of level {}, or higher. ", spdlog::level::to_string_view(consoleLevel))));
	std::clog << "logger/debug: " << logHeader << std::endl;
	spdlog::filename_t filename = getLogFilename_new().u8string();
	
// 	auto stderror_graber_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>();
	m_sink = std::make_shared<spdlog::sinks::dist_sink_mt>();
	
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	stdout_sink->set_color(spdlog::level::critical, logger_colors(bold_on_red)); // Error.
	stdout_sink->set_color(spdlog::level::err, logger_colors(yellow_bold)); // Warning.
	stdout_sink->set_color(spdlog::level::warn, logger_colors(green)); // Notice.
	stdout_sink->set_color(spdlog::level::info, logger_colors(white));
	stdout_sink->set_color(spdlog::level::debug, logger_colors(blue));
	stdout_sink->set_color(spdlog::level::trace, logger_colors(cyan));

	spdlog::file_event_handlers handlers;
	handlers.after_open = [logHeader](spdlog::filename_t filename, std::FILE *fstream) { writeLogHeader(filename, fstream, logHeader); };

	m_sink->add_sink(stdout_sink);
	m_sink->set_pattern("[%T]:::%^%n / %l%$::: %v");
	m_sink->set_level(spdlog::level::trace);

	m_defaultLogger = std::make_shared<spdlog::async_logger>(LogSystem{LogSystem::LOGGER}.toString(), m_sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	m_defaultLogger->set_level(spdlog::level::trace);

	auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, 1024 * 1024 * 2, 5, true, handlers);
	m_sink->add_sink(file_sink);

	auto headerLogger = std::make_shared<spdlog::async_logger>(PACKAGE, stdout_sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	headerLogger->log(spdlog::level::warn, logHeader);
	
	for (const auto& system: LogSystem()) {
		if (system == LogSystem::LOGGER) continue;
		std::unique_lock lock(m_LoggerRegistryMutex);
		auto newLogger = m_defaultLogger->clone(system);
		spdlog::register_logger(newLogger);
		builtLoggers.try_emplace(system, newLogger);
		newLogger->log(spdlog::level::trace, fmt::format("Logger subsystem initialized, system: {}", system));
	}
	
	grabber = std::make_unique<StderrGrabber>();
}

void SpdLogger::writeLogHeader(spdlog::filename_t filename, std::FILE* fd, std::string header) {
	fs::path _filename{filename};
	if (fs::exists(_filename) && fs::file_size(_filename) >30) {
		trace(LogSystem::LOGGER, "Not writing header to {}. File is not empty, probably a previous log being rotated.", filename);
		std::clog << "logger/debug: Not writing header to " << filename << ". Size > 0, so probably a log being rotated." << std::endl; 
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
	notice(LogSystem::LOGGER, "More details might be available in {}", getLogFilename_new().u8string());
	spdlog::shutdown();
}

const loggerPtr SpdLogger::getLogger(LogSystem loggerName) {
	if (loggerName == LogSystem::LOGGER) {
		return m_defaultLogger;
	}
	loggerPtr ret;
	Profiler prof("getLogger");
	try {
		std::shared_lock lock(m_LoggerRegistryMutex);
		ret = builtLoggers.at(loggerName);
	} catch (std::out_of_range const&) {
		// logger not found, did we already build it?
		auto ptr = spdlog::get(loggerName.toString());
		if (ptr) {
			std::unique_lock lock(m_LoggerRegistryMutex);
			builtLoggers.try_emplace(loggerName, ptr);
			ret = builtLoggers.at(loggerName);
		}
		else {
			std::unique_lock lock(m_LoggerRegistryMutex);
			auto newLogger = std::make_shared<spdlog::async_logger>(loggerName, m_sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
			spdlog::register_logger(newLogger);
			builtLoggers.try_emplace(loggerName, newLogger);
			ret = newLogger;
		}
	}
	if (ret == nullptr) {
		throw std::runtime_error(fmt::format("Couldn't find nor construct logger for subsystem={}", loggerName.toString()));
	}
	return ret;
}
