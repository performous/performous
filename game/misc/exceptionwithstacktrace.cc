#include "exceptionwithstacktrace.hh"

ExceptionWithStackTrace::ExceptionWithStackTrace(std::string const& message)
: m_message(message), m_trace(StackFrame::getStackTrace(1)) {
}

char const* ExceptionWithStackTrace::what() const noexcept {
	return m_message.c_str();
}

StackTrace const& ExceptionWithStackTrace::getStackTrace() const {
	return m_trace;
}


