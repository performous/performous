#pragma once

#include <iostream>
#include <map>
#include <string>
#include <unicode/errorcode.h>
#include <unicode/tblcoll.h>
#include <unicode/uclean.h>
#include <unicode/ucsdet.h>

typedef std::map<std::string, std::string> songMetadata;

class UnicodeUtil {
	static icu::ErrorCode m_staticIcuError;
	enum class CaseMapping { LOWER, UPPER, TITLE, NONE };

	public:
	UnicodeUtil() = delete;
	~UnicodeUtil() = delete;
	static void collate (songMetadata& stringmap);
	static std::string getCharset(std::string_view str);
	static std::string convertToUTF8 (std::string_view str, std::string _filename = std::string(), CaseMapping toCase = CaseMapping::NONE, bool assumeUTF8 = false);
	static bool removeUTF8BOM(std::string_view str);
	static bool caseEqual (std::string_view lhs, std::string_view rhs, bool assumeUTF8 = false);
	static bool isRTL(std::string_view str); ///< FIXME: This won't be used for now, but it might be useful if we eventually implement RTL translations. As-is, at least on my mac, Performous is refusing to render Arabic text, although it might be a font issue.
	static std::string toLower (std::string_view str);
	static std::string toUpper (std::string_view str);
	static std::string toTitle (std::string_view str);
	static icu::RuleBasedCollator m_searchCollator;
	static icu::RuleBasedCollator m_sortCollator;
};
