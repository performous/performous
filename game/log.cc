#include "log.hh"

#include "fs.hh"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>

/** \file
 * \brief The std::clog logger.
 *
 * General message format: <tt>subsystem/level: message</tt>
 *
 * Example:
 * \code
 * std::clog << "foo/info: Here's an info message from subsystem foo" << std::endl;
 * \endcode
 *
 * Each message may contain newlines and flushing the stream (i.e. by std::endl or std::flush) must be done
 * when and only when the message is complete.
 *
 * Any lower-case subsystem name including hyphens may be used. The levels, in descending order of priority
 * are as follows:
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
 * For thread-safety, messages must be written as a single string, e.g.
 *   std::clog << "sys/info: message " + std::to_string(num) + "\n" << std::flush;
 *
 * For multi-line formatted messages, \r may be used in place of inner newlines to avoid repeating the prefix.
 *
 **/

struct StreamRedirect {
	std::ios& stream;
	std::streambuf* backup;
	StreamRedirect(std::ios& stream, std::streambuf* other): stream(stream), backup(stream.rdbuf()) { stream.rdbuf(other); }
	~StreamRedirect() { stream.rdbuf(backup); }
};

// Capture stderr spam from other libraries and log it properly
// Note: std::cerr retains its normal functionality but other means of writing stderr get redirected to std::clog
#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <future>
struct StderrGrabber {
	struct FD: boost::noncopyable {
		int fd = -1;
		FD() = default;
		FD(FD&& other): fd(other.fd) { other.fd = -1; }
		~FD() { if (fd != -1) (void)close(fd); }
		operator int&() { return fd; }
	};
	struct Pipe {
		FD recv, send;
		Pipe() { if (pipe(reinterpret_cast<int*>(this)) != 0) throw std::runtime_error("pipe()"); }
	};
	static_assert(std::is_standard_layout<Pipe>::value, "struct Pipe must use standard layout");
	std::future<void> logger;
	/// Duplicate the real stderr into another fd and bind that to boost iostream, restore on exit
	struct Stream: boost::iostreams::stream<boost::iostreams::file_descriptor_sink> {
		Stream(): stream(dup(STDERR_FILENO), boost::iostreams::close_handle) {}
		~Stream() { (void)dup2((*this)->handle(), STDERR_FILENO); }
	} stream;
	StreamRedirect redirect{ std::cerr, stream.rdbuf() };  // Make std::cerr output to our copy of real stderr
	StderrGrabber() {
		Pipe pipefd;
		// Replace STDERR_FILENO with a duplicate of pipefd.send
		if (dup2(pipefd.send, STDERR_FILENO) == -1) throw std::runtime_error("dup2()");
		std::clog << "stderr/info: Standard error output redirected here\n" << std::flush;
		// Start a thread that logs messages from pipe; this is terminated when ~Stream restores STDERR_FILENO (closing the sending end)
		logger = std::async(std::launch::async, [fd = std::move(pipefd.recv)]() mutable {
			std::string line;
			unsigned count = 0;
			for (char ch; read(fd, &ch, 1) == 1;) {
				line += ch;
				if (ch != '\n') continue;
				std::clog << "stderr/info: " + line << std::flush;
				line.clear(); ++count;
			}
			if (count > 0) std::clog << "stderr/notice: " + std::to_string(count) + " messages logged to stderr/info\n" << std::flush;
		});
	}
	~StderrGrabber() { close(STDERR_FILENO); }
};
#else
struct StderrGrabber {};  // Not supported on Windows
#endif

/// Receives strings written to boost::iostreams::stream_buffer<VerboseMessageSink>
struct VerboseMessageSink: boost::iostreams::sink {
	std::streamsize write(const char* s, std::streamsize n);
};

static struct Impl {
	const std::map<std::string, int> numeric = {
		{ "debug", 0 },
		{ "info", 1 },
		{ "notice", 2 },
		{ "warning", 3 },
		{ "error", 4 },
	};
	std::mutex mutex;
	using Lock = std::lock_guard<std::mutex>;
	struct Redirects {
		VerboseMessageSink vms;
		boost::iostreams::stream_buffer<VerboseMessageSink> sb{vms};
		StreamRedirect clogRedirect{std::clog, &sb};
		StderrGrabber grabber;
	};
	std::unique_ptr<Redirects> redirects;
	fs::ofstream file;
	std::string target;
	int minLevel;
	std::string buffer;
	void parse(std::string&& text) {
		Lock l(mutex);
		buffer += text;
		// Split into lines
		for (std::string::size_type pos; (pos = buffer.find('\n')) != std::string::npos; ) {
			++pos;
			parseLine(buffer.substr(0, pos));
			buffer.erase(0, pos);
		}
	}
	void parseLine(std::string&& line) {
		// Parse prefix as subsystem/level:...
		auto slash = line.find('/');
		auto colon = line.find(": ", slash);
		if (slash == std::string::npos || colon == std::string::npos) {
			parseLine("logger/error: Invalid log prefix on line: " + line);
			return;
		}
		std::string subsystem(line, 0, slash);
		std::string level(line, slash + 1, colon - slash - 1);
		auto num = numeric.find(level);
		if (num == numeric.end()) {
			parseLine("logger/error: Invalid level '" + level + "' line: " + line);
			return;
		}
		// Write to file & stderr those messages that pass the filtering
		if (num->second < minLevel && (target.empty() || subsystem.find(target) == std::string::npos)) return;
		for (char& ch: line) if (ch == '\r') ch = '\n';  // Hack for multi-line log messages
		std::cerr << line << std::flush;
		file << line << std::flush;
	}

} self;

std::streamsize VerboseMessageSink::write(const char* s, std::streamsize n) {
	self.parse(std::string(s, n)); // Note: s is *not* a c-string, thus we must stop after n chars.
	return n;
}

Logger::Logger(std::string const& level) {
	if (self.redirects) throw std::logic_error("Multiple loggers constructed. There can only be one.");
	if (level.find_first_of(":/_* ") != std::string::npos) throw std::runtime_error("Invalid logging level specified. Specify either a subsystem name (e.g. logger) or a level (debug, info, notice, warning, error).");
	pathBootstrap();  // So that log filename is known...
	std::string msg = "logger/notice: Logging ";
	if (level.empty()) {
		self.minLevel = 2;  // Display all notices, warnings and errors
		msg += "all notices, warnings and errors.";
	} else if (level == "none") {
		self.minLevel = 100;
		msg += "disabled.";  // No-one will see this, so what's the point? :)
	} else {
		auto num = self.numeric.find(level);
		if (num == self.numeric.end() /* Not a valid level name */) {
			self.minLevel = 4;  // Display errors from any subsystem
			self.target = level;  // All messages from the given subsystem
			msg += "everything from subsystem " + self.target + " and all errors.";
		} else {
			self.minLevel = num->second;
			msg += "any events of " + level + " or higher level.";
		}
	}
	if (self.minLevel < 100) {
		fs::path name = getLogFilename();
		fs::create_directories(name.parent_path());
		self.file.open(name);
		msg += " Log file: " + name.string();
	}
	self.redirects = std::make_unique<Impl::Redirects>();
	atexit(Logger::teardown);
	std::clog << msg << std::endl;
}

Logger::~Logger() { teardown(); }

void Logger::teardown() {
	if (!self.redirects) return;
	std::clog << "logger/info: Exiting normally.\n" << std::flush;
	self.redirects.reset();
	self.file.close();
}
