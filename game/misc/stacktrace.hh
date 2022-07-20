#pragma once

#include <string>
#include <vector>
#include <iostream>

class StackFrame;

using StackTrace = std::vector<StackFrame>;

class StackFrame {
public:
	static StackTrace getStackTrace(size_t skipFrames = 0);

	std::string const& getInfo() const;

private:
	StackFrame(std::string const& info);

private:
	std::string m_info;
};

std::ostream& operator<<(std::ostream& os, StackTrace const& stack);
