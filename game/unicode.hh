#pragma once

#include <iostream>
#include <map>
#include <string>
#include <unicode/tblcoll.h>
#include <unicode/uclean.h>
#include <unicode/ucsdet.h>

typedef std::map<std::string, std::string> songMetadata;

class UnicodeUtil {
	static UErrorCode m_staticIcuError;
	enum class CaseMapping { LOWER, UPPER, TITLE, NONE };

	public:
	UnicodeUtil() {}
	~UnicodeUtil() {};
	static void collate (songMetadata& stringmap);
	static std::string getCharset(std::string const& str);
	static std::string convertToUTF8 (std::string const& str, std::string _filename = std::string(), CaseMapping toCase = CaseMapping::NONE, bool assumeUTF8 = false);
	static bool caseEqual (std::string const& lhs, std::string const& rhs, bool assumeUTF8 = false);
	static std::string toLower (std::string const& str);
	static std::string toUpper (std::string const& str);
	static std::string toTitle (std::string const& str);
	static icu::RuleBasedCollator m_dummyCollator;
	static icu::RuleBasedCollator m_sortCollator;
};
