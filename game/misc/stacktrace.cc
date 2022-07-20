#include "stacktrace.hh"

#include <execinfo.h>
#include <cxxabi.h>

StackTrace StackFrame::getStackTrace(size_t skipFrames) {
	void* addrlist[256];

	auto addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

	if (addrlen == 0)
	return {};

	auto result = std::vector<StackFrame>();
	auto symbollist = backtrace_symbols(addrlist, addrlen);
	size_t funcnamesize = 256;
	char* funcname = (char*)malloc(funcnamesize);

	for (auto i = 1 + skipFrames; i < addrlen; ++i) {
		//std::cout << i << ": " << symbollist[i] << std::endl;
		char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

		// find parentheses and +address offset surrounding the mangled name:
		// ./module(function+0x15c) [0x8048a6d]
		for (char *p = symbollist[i]; *p; ++p) {
			if (*p == '(')
				begin_name = p;
			else if (*p == '+')
				begin_offset = p;
			else if (*p == ')' && begin_offset) {
				end_offset = p;
				break;
			}
		}

		if (begin_name && begin_offset && end_offset && begin_name < begin_offset)
		{
			*begin_name++ = '\0';
			*begin_offset++ = '\0';
			*end_offset = '\0';

			// mangled name is now in [begin_name, begin_offset) and caller
			// offset in [begin_offset, end_offset). now apply
			// __cxa_demangle():

			int status;
			char* ret = abi::__cxa_demangle(begin_name, funcname, &funcnamesize, &status);
			if (status == 0) {
				result.emplace_back(StackFrame(ret));
				funcname = ret;
			}
			else {
				// demangling failed. Output function name as a C function with  no arguments.
				result.emplace_back(StackFrame(begin_name));
			}
		}
		else {
			// couldn't parse the line? print the whole line.
			result.emplace_back(StackFrame(symbollist[i]));
		}
	}

	free(funcname);
	free(symbollist);

	return result;
}

StackFrame::StackFrame(std::string const& info)
: m_info(info) {
}

std::string const& StackFrame::getInfo() const {
   return m_info;
}


std::ostream& operator<<(std::ostream& os, std::vector<StackFrame> const& stack) {
	for(auto const& frame : stack)
		os << frame.getInfo() << std::endl;

	return os;
}
