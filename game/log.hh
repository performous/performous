#pragma once

#include "logger_subsystem.hh"
#include "platform.hh"

#include <string_view>
using namespace std::string_view_literals;
#define SPDLOG_LEVEL_NAMES { "TRACE"sv, "DEBUG"sv, "INFO"sv, "NOTICE"sv, "WARNING"sv, "ERROR"sv, "OFF"sv }

#if (BOOST_OS_WINDOWS)
#include <cstdint>

// These values were gotten from wincon.h

#define LOGGER_BLUE 0x0001
#define LOGGER_GREEN 0x0002
#define LOGGER_RED 0x0004
#define LOGGER_INTENSITY 0x0008
#define LOGGER_BACKGROUND(color) color<<4

using LoggerColor = std::uint16_t;

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
#else
#include <string_view>
#include <spdlog/sinks/ansicolor_sink.h>
using LoggerColor = std::string_view;
#define logger_colors(color) stdout_sink->color
#endif

#include <fmt/format.h>
#include <spdlog/async.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using loggerPtr = std::shared_ptr<spdlog::logger>;

class Logger {
public:
	Logger(std::string const& level);
	~Logger();
	static void teardown();
};

template<>
struct fmt::formatter<LogSystem> : fmt::formatter<std::string> {
	auto format(LogSystem subsystem, format_context &ctx) const -> decltype(ctx.out())
	{
		return format_to(ctx.out(), "{}", subsystem.toString());
	}
};

class SpdLogger {
  public:
	SpdLogger(spdlog::level::level_enum const& consoleLevel = spdlog::level::info);
	~SpdLogger();

	template <typename... Args>
	static void log(LogSystem subsystem, spdlog::level::level_enum level, Args &&...args) {
		auto logger = getLogger(subsystem);
		logger->log(level, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void error(LogSystem subsystem, Args &&...args) { log(subsystem, spdlog::level::critical, std::forward<Args>(args)...); }
	
	template <typename... Args>
	static void warn(LogSystem subsystem, Args &&...args) { log(subsystem, spdlog::level::err, std::forward<Args>(args)...); }

	template <typename... Args>
	static void notice(LogSystem subsystem, Args &&...args) { log(subsystem, spdlog::level::warn, std::forward<Args>(args)...); }
	
	template <typename... Args>
	static void info(LogSystem subsystem, Args &&...args) { log(subsystem, spdlog::level::info, std::forward<Args>(args)...); }

	template <typename... Args>
	static void debug(LogSystem subsystem, Args &&...args) { log(subsystem, spdlog::level::debug, std::forward<Args>(args)...); }
	
	template <typename... Args>
	static void trace(LogSystem subsystem, Args &&...args) { log(subsystem, spdlog::level::trace, std::forward<Args>(args)...); }


  private:
	static std::unordered_map<LogSystem, loggerPtr> builtLoggers;
	static const loggerPtr getLogger(LogSystem);
	static std::shared_ptr<spdlog::sinks::dist_sink_mt> m_sink;
	static std::shared_mutex m_LoggerRegistryMutex;
	static loggerPtr m_defaultLogger;
	static void writeLogHeader(spdlog::filename_t filename, std::FILE* fd, std::string header);
};
