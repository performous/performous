#pragma once

#include "logger_subsystem.hh"
#include "platform.hh"

#include <fmt/format.h>

#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std::string_view_literals;

#define SPDLOG_LEVEL_NAMES { "TRACE"sv, "DEBUG"sv, "INFO"sv, "NOTICE"sv, "WARNING"sv, "ERROR"sv, "OFF"sv }

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

#if (BOOST_OS_WINDOWS)
#include <cstdint>

// These values were gotten from wincon.h

#define LOGGER_BLUE 0x0001
#define LOGGER_GREEN 0x0002
#define LOGGER_RED 0x0004
#define LOGGER_INTENSITY 0x0008
#define LOGGER_BACKGROUND(color) color<<4

using LoggerColor = std::uint16_t;

#else
#include <spdlog/sinks/ansicolor_sink.h>
using LoggerColor = std::string_view;
#define logger_colors(color) stdout_sink->color
#endif

using LoggerPtr = std::shared_ptr<spdlog::logger>;

template<>
struct fmt::formatter<LogSystem> : fmt::formatter<std::string> {
	auto format(LogSystem subsystem, format_context &ctx) const -> decltype(ctx.out())
	{
		return format_to(ctx.out(), "{}", subsystem.toString());
	}
};

struct StderrGrabber;

class SpdLogger {
  public:
	SpdLogger(spdlog::level::level_enum const& consoleLevel = spdlog::level::info);
	~SpdLogger();

	template <typename... Args>
	static void log(LogSystem::Values subsystem, spdlog::level::level_enum level, Args &&...args) {
		LoggerPtr logger;
		try {
			logger = getLogger(subsystem);
		}
		catch (std::runtime_error const& e) {
			logger = m_defaultLogger;
			logger->log(spdlog::level::critical, e.what());
		}
		logger->log(level, std::forward<Args>(args)...);
	}

	static void toggleProfilerLogger();

	template <typename... Args>
	static void error(LogSystem::Values subsystem, Args &&...args) { log(subsystem, spdlog::level::critical, std::forward<Args>(args)...); }
	
	// For convenience, let's use both.
	template <typename... Args>
	static void warn(LogSystem::Values subsystem, Args &&...args) { log(subsystem, spdlog::level::err, std::forward<Args>(args)...); }
	template <typename... Args>
	static void warning(LogSystem::Values subsystem, Args &&...args) { log(subsystem, spdlog::level::err, std::forward<Args>(args)...); }

	template <typename... Args>
	static void notice(LogSystem::Values subsystem, Args &&...args) { log(subsystem, spdlog::level::warn, std::forward<Args>(args)...); }
	
	template <typename... Args>
	static void info(LogSystem::Values subsystem, Args &&...args) { log(subsystem, spdlog::level::info, std::forward<Args>(args)...); }

	template <typename... Args>
	static void debug(LogSystem::Values subsystem, Args &&...args) { log(subsystem, spdlog::level::debug, std::forward<Args>(args)...); }
	
	template <typename... Args>
	static void trace(LogSystem::Values subsystem, Args &&...args) { log(subsystem, spdlog::level::trace, std::forward<Args>(args)...); }

	static bool initialized() { return m_defaultLogger != nullptr; }

	inline static const std::string newLineDec = "             └---"; // new line decorator.
  private:
	inline static const std::string formatString{"[%T]:::%^%n / %l%$::: %v"};
	static std::unordered_map<LogSystem, LoggerPtr> builtLoggers;
	static LoggerPtr getLogger(LogSystem::Values const& loggerName);
	static std::shared_ptr<spdlog::sinks::basic_file_sink_mt> m_profilerSink;
	static std::shared_ptr<spdlog::sinks::dist_sink_mt> m_sink;
	static std::shared_mutex m_LoggerRegistryMutex;
	static LoggerPtr m_defaultLogger;
	static LoggerPtr m_ProfilerLogger;
	static void writeLogHeader(spdlog::filename_t filename, std::FILE* fd, std::string header);
	static void initializeSinks(spdlog::level::level_enum const& consoleLevel = spdlog::level::info);
	static LoggerPtr constructLogger(const LogSystem system);
	static std::unique_ptr<StderrGrabber> grabber;

};
