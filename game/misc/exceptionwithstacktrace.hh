#pragma once

#include "stacktrace.hh"

#include <stdexcept>

class ExceptionWithStackTrace : public std::exception {
public:
	ExceptionWithStackTrace(std::string const& message);
	~ExceptionWithStackTrace() override = default;

	char const* what() const noexcept override;

	StackTrace const& getStackTrace() const;

private:
	std::string const m_message;
	StackTrace const m_trace;
};


