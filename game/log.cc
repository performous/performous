#include "log.hh"

#include "fs.hh"
#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/thread/mutex.hpp>
#include <iostream>
#include <stdexcept>

/** \file
 * \brief The std::clog logger.
 *
 * The classification is \e discretionary, meaning that you can specify any message class you want but there are only 4 recommended classes: \p debug, \p info, \p warning and \p error.
 * \attention Each new line (ended by <tt>std::endl</tt>) must be prefixed with the message classification. New lines by <tt>"\n"</tt> will belong to the previous line as far as the filter is concerned.
 *
 * The default log level is specified by logger::default_log_level.
 *
 * General message format: <tt>subsystem/class: message</tt>
 *
 * Example:
 * \code
 * std::clog << "subsystem/info: Here's an info message from subsystem" << std::endl;
 * \endcode
 *
 * \todo setup and teardown could probably be RAII-ized away by making a class and having fun by making them dtor/ctor. (it'd be nice if we could make it init itself before any other component that might use it does)
 **/

/** \internal
 * Guard to ensure we're atomically printing to cout/cerr.
 * \attention This only guards from multiple clog interleaving, not other console I/O.
 */
boost::mutex log_lock;

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
	boost::mutex::scoped_lock l(log_lock);
	std::cerr << msg << std::flush;
	file << msg << std::flush;
}

int numeric(std::string const& level) {
	if (level == "debug") return 0;
	if (level == "info") return 1;
	if (level == "warning") return 2;
	if (level == "error") return 3;
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
	if (level.find('/') != std::string::npos) throw std::runtime_error("Invalid logging level specified. Specify either a subsystem name or a level (debug, info, warning, error).");
	pathBootstrap();  // So that log filename is known...
	boost::mutex::scoped_lock l(log_lock);
	if (level.empty()) {
		minLevel = 2;  // Display all warnings and errors
	} else if (level == "none") {
		minLevel = 100;
	} else {
		minLevel = numeric(level);
		if (minLevel == -1 /* Not a valid level name */) {
			minLevel = 3;  // Display errors from any subsystem
			target = level;  // All messages from the given subsystem
		}
	}
	if (minLevel < 100) file.open(getLogFilename());
	sb.open(vsm);
	default_ClogBuf = std::clog.rdbuf();
	std::clog.rdbuf(&sb);
	atexit(Logger::teardown);
}

Logger::~Logger() { teardown(); }

void Logger::teardown() {
	boost::mutex::scoped_lock l(log_lock);
	if (!default_ClogBuf) return;
	std::clog.rdbuf(default_ClogBuf);
	sb.close();
	file.close();
	default_ClogBuf = nullptr;
}

