#pragma once

#include <string>

class Logger {
public:
	Logger(std::string const& level);
	~Logger();
	static void teardown();
};

