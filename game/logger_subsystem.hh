#pragma once

#include <iterator>
#include <string>

class SpdLogger;

class LogSystem {
  public:
	// The actual underlying values are not really important. These are just so we don't accidentally construct or try to use loggers for inexistent subsystems. If you add or remove a value, for the love of god keep them sorted, and add the corresponding case to the switch statement below.
	enum Values : std::size_t {
		BEGIN = 0,
		AUDIO = BEGIN,
		CACHE,
		CONFIG,
		CONTROLLERS,
		DANCING,
		DATABASE,
		ENGINE,
		FFMPEG,
		FILESYSTEM,
		I18N,
		IMAGE,
		INSTRUMENTS,
		JSON,
		LOGGER,
		MIDI,
		OPENGL,
		PROFILER,
		SINGING,
		SONGPARSER,
		SONGS,
		STDERR,
		TEXT,
		WEBCAM,
		WEBSERVER,
		COUNT // Sentinel value for iteration
	};

  private:
	Values value;

  public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = LogSystem;
	using difference_type = std::ptrdiff_t;
	using pointer = LogSystem*;
	using reference = LogSystem&;

	LogSystem() : value(static_cast<Values>(0)) {}
	LogSystem(Values val) : value(val) {}
	LogSystem begin() const { return LogSystem(LogSystem::Values::AUDIO); }
	LogSystem end() const { return LogSystem(LogSystem::Values::COUNT); }

	LogSystem operator*() const { return *this; }

	bool operator!=(const LogSystem& other) const {
		return value != other.value;
	}

	bool operator!=(const Values& other) const {
		return value != other;
	}

	LogSystem& operator++() {
		if (value != Values::COUNT) {
			// Increment until COUNT
			value = static_cast<Values>(static_cast<std::size_t>(value) + 1);
		}
		return *this;
	}
	
	operator std::string() const { return toString(); }
	operator Values() const { return value; }

	friend std::ostream& operator<< (std::ostream&	out, LogSystem const& sys) {
		out << sys.toString();
		return out;
	}
	std::string toString() const {
		return subsystemToString(value);
	}
	friend std::string subsystemToString(LogSystem::Values const& val) {
		std::string subsystem_string("UNKNOWN");
		switch(val) {
			case LogSystem::Values::AUDIO:
				subsystem_string = "AUDIO";
				break;
			case LogSystem::Values::CACHE:
				subsystem_string = "CACHE";
				break;
			case LogSystem::Values::CONFIG:
				subsystem_string = "CONFIG";
				break;
			case LogSystem::Values::CONTROLLERS:
				subsystem_string = "CONTROLLERS";
				break;
			case LogSystem::Values::DANCING:
				subsystem_string = "DANCING";
				break;
			case LogSystem::Values::DATABASE:
				subsystem_string = "DATABASE";
				break;
			case LogSystem::Values::ENGINE:
				subsystem_string = "ENGINE";
				break;
			case LogSystem::Values::FFMPEG:
				subsystem_string = "FFMPEG";
				break;
			case LogSystem::Values::FILESYSTEM:
				subsystem_string = "FILESYSTEM";
				break;
			case LogSystem::Values::I18N:
				subsystem_string = "I18N";
				break;
			case LogSystem::Values::IMAGE:
				subsystem_string = "IMAGE";
				break;
			case LogSystem::Values::INSTRUMENTS:
				subsystem_string = "INSTRUMENTS";
				break;
			case LogSystem::Values::JSON:
				subsystem_string = "JSON";
				break;
			case LogSystem::Values::LOGGER:
				subsystem_string = "LOGGER";
				break;
			case LogSystem::Values::MIDI:
				subsystem_string = "MIDI";
				break;
			case LogSystem::Values::OPENGL:
				subsystem_string = "OPENGL";
				break;
			case LogSystem::Values::PROFILER:
				subsystem_string = "PROFILER";
				break;
			case LogSystem::Values::SINGING:
				subsystem_string = "SINGING";
				break;
			case LogSystem::Values::SONGPARSER:
				subsystem_string = "SONGPARSER";
				break;
			case LogSystem::Values::SONGS:
				subsystem_string = "SONGS";
				break;
			case LogSystem::Values::STDERR:
				subsystem_string = "STDERR";
				break;
			case LogSystem::Values::TEXT:
				subsystem_string = "TEXT";
				break;
			case LogSystem::Values::WEBCAM:
				subsystem_string = "WEBCAM";
				break;
			case LogSystem::Values::WEBSERVER:
				subsystem_string = "WEBSERVER";
				break;
			default: break;
		}
		return subsystem_string;
	}
	friend std::hash<LogSystem>;
	friend class SpdLogger;
};

template<>
struct std::hash<LogSystem>
{
	std::size_t operator()(const LogSystem& sys) const noexcept {
	std::size_t baseHash = std::hash<std::string>{}("PACKAGE_LogSystem");
	std::size_t enumHash = std::hash<std::size_t>{}(sys.value);
	return baseHash * enumHash;
	}
};