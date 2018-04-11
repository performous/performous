#include "log.hh"

#include "fs.hh"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <iostream>
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
 **/

struct StreamRedirect {
	std::ios& stream;
	std::streambuf* backup;
	StreamRedirect(std::ios& stream, std::ios& other): stream(stream), backup(stream.rdbuf()) { stream.rdbuf(other.rdbuf()); }
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
	StreamRedirect redirect{ std::cerr, stream };  // Make std::cerr output to our copy of real stderr
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
			if (count > 0) std::clog << "stderr/notice: " << count << " messages logged to stderr/info\n" << std::flush;
		});
	}
};
#else
struct StderrGrabber {};  // Not supported on Windows
#endif

/** \internal The implementation of the stream filter that handles the message filtering. **/
class VerboseMessageSink : public boost::iostreams::sink {
  public:
	std::streamsize write(const char* s, std::streamsize n);
};

static struct Impl {
	/** \internal
	* Guard to ensure we're atomically printing to cerr.
	* \attention This only guards from multiple clog interleaving, not other console I/O.
	*/
	std::mutex log_lock;
	std::unique_ptr<StderrGrabber> grabber;
	// defining them in main() causes segfault at exit as they apparently got free'd before we're done using them
	boost::iostreams::stream_buffer<VerboseMessageSink> sb; //!< \internal
	VerboseMessageSink vsm; //!< \internal

	std::unique_ptr<StreamRedirect> clogRedirect;
	std::streambuf* default_ClogBuf;
	fs::ofstream file;

	std::string target;
	int minLevel;
	void writeLog(std::string const& msg) {
		std::lock_guard<std::mutex> l(log_lock);
		std::cerr << msg << std::flush;
		file << msg << std::flush;
	}


} self;

int numeric(std::string const& level) {
	if (level == "debug") return 0;
	if (level == "info") return 1;
	if (level == "notice") return 2;
	if (level == "warning") return 3;
	if (level == "error") return 4;
	return -1;
}

std::streamsize VerboseMessageSink::write(const char* s, std::streamsize n) {
	std::string line(s, n);  // Note: s is *not* a c-string, thus we must stop after n chars.
	// Parse prefix as subsystem/level:...
	size_t slash = line.find('/');
	size_t colon = line.find(": ", slash);
	if (slash == std::string::npos || colon == std::string::npos) {
		std::string msg = "logger/error: Invalid log prefix on line [[[\n" + line + "]]]\n";
		write(msg.data(), msg.size());
		return n;
	}
	std::string subsystem(line, 0, slash);
	std::string level(line, slash + 1, colon - slash - 1);
	int lev = numeric(level);
	if (lev == -1) {
		std::string msg = "logger/error: Invalid level '" + level + "' line [[[\n" + line + "]]]\n";
		write(msg.data(), msg.size());
		return n;
	}
	if (lev >= self.minLevel || (!self.target.empty() && subsystem.find(self.target) != std::string::npos)) {
		self.writeLog(line);
	}
	return n;
}

Logger::Logger(std::string const& level) {
	if (self.default_ClogBuf) throw std::logic_error("Multiple loggers constructed. There can only be one.");
	if (level.find_first_of(":/_* ") != std::string::npos) throw std::runtime_error("Invalid logging level specified. Specify either a subsystem name (e.g. logger) or a level (debug, info, notice, warning, error).");
	pathBootstrap();  // So that log filename is known...
	std::string msg = "logger/notice: Logging ";
	{
		std::lock_guard<std::mutex> l(self.log_lock);
		if (level.empty()) {
			self.minLevel = 2;  // Display all notices, warnings and errors
			msg += "all notices, warnings and errors.";
		} else if (level == "none") {
			self.minLevel = 100;
			msg += "disabled.";  // No-one will see this, so what's the point? :)
		} else {
			self.minLevel = numeric(level);
			if (self.minLevel == -1 /* Not a valid level name */) {
				self.minLevel = 4;  // Display errors from any subsystem
				self.target = level;  // All messages from the given subsystem
				msg += "everything from subsystem " + self.target + " and all errors.";
			} else {
				msg += "any events of " + level + " or higher level.";
			}
		}
		if (self.minLevel < 100) {
			fs::path name = getLogFilename();
			fs::create_directories(name.parent_path());
			self.file.open(name);
			msg += " Log file: " + name.string();
		}
		self.sb.open(self.vsm);
		self.default_ClogBuf = std::clog.rdbuf();
		std::clog.rdbuf(&self.sb);
		atexit(Logger::teardown);
	}
	std::clog << msg << std::endl;
	self.grabber = std::make_unique<StderrGrabber>();
}

Logger::~Logger() { teardown(); }

void Logger::teardown() {
	self.grabber.reset();
	if (self.default_ClogBuf) std::clog << "logger/info: Exiting normally." << std::endl;
	std::lock_guard<std::mutex> l(self.log_lock);
	if (!self.default_ClogBuf) return;
	std::clog.rdbuf(self.default_ClogBuf);
	self.sb.close();
	self.file.close();
	self.default_ClogBuf = nullptr;
}

