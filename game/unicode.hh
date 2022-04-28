#pragma once

#include <iostream>
#include <map>
#include <string>
#include <unicode/errorcode.h>
#include <unicode/tblcoll.h>
#include <unicode/uclean.h>
#include <unicode/ucsdet.h>

typedef std::map<std::string, std::string> songMetadata;

struct UnicodeUtil {
	UnicodeUtil() {}
	~UnicodeUtil() {};
	static void collate (songMetadata& stringmap);
	static std::string getCharset(std::string const& str);
	static bool isRTL(std::string const& str); ///< FIXME: This won't be used for now, but it might be useful if we eventually implement RTL translations. As-is, at least on my mac, Performous is refusing to render Arabic text, although it might be a font issue.
	static void convertToUTF8 (std::stringstream &_stream, std::string _filename);
	static std::string convertToUTF8 (std::string const& str);
	static std::string toLower (std::string const& str, size_t length = 0);
	static std::string toUpper (std::string const& str, size_t length = 0);
	static icu::RuleBasedCollator m_dummyCollator;
	static icu::RuleBasedCollator m_sortCollator;
	static icu::ErrorCode m_staticIcuError;
};
