#pragma once

#include <string>

namespace logger {
	/** Initialize the logger facilities **/
	void setup(const std::string &level_regexp);

	/** Tear down the logger facilities **/
	void teardown();

	//! This variable selected the default log level.
	const char* const default_log_level=".*/error";

	/** \test Runs some debug tests \internal **/
	void __log_hh_test();
}
