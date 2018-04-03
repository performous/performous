#include "log.hh"

#include "fs.hh"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <iostream>
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

/** \internal
 * Guard to ensure we're atomically printing to cerr.
 * \attention This only guards from multiple clog interleaving, not other console I/O.
 */
std::mutex log_lock;

/** \internal The implementation of the stream filter that handles the message filtering. **/
class VerboseMessageSink : public boost::iostreams::sink {
  public:
	std::streamsize write(const char* s, std::streamsize n);
};

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
	if (lev >= minLevel || (!target.empty() && subsystem.find(target) != std::string::npos)) {
		writeLog(line);
	}
	return n;
}

Logger::Logger(std::string const& level) {
	if (default_ClogBuf) throw std::logic_error("Multiple loggers constructed. There can only be one.");
	if (level.find_first_of(":/_* ") != std::string::npos) throw std::runtime_error("Invalid logging level specified. Specify either a subsystem name or a level (debug, info, notice, warning, error).");
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

Logger::~Logger() { teardown(); }

void Logger::teardown() {
	if (default_ClogBuf) std::clog << "logger/info: Exiting normally." << std::endl;
	std::lock_guard<std::mutex> l(log_lock);
	if (!default_ClogBuf) return;
	std::clog.rdbuf(default_ClogBuf);
	sb.close();
	file.close();
	default_ClogBuf = nullptr;
}

