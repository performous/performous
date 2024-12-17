#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unicode/errorcode.h>
#include <unicode/tblcoll.h>
#include <unicode/uclean.h>
#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>

typedef std::map<std::string, std::string> songMetadata;

class UnicodeString;

struct Converter {
	/// Constructor/Destructor
	Converter(std::string const& codepage);
	Converter(Converter&& c) noexcept;
	Converter(Converter& c) = delete;
	Converter(Converter const& c) = delete;	

	icu::UnicodeString convertToUTF8(std::string_view sv); ///< Do the actual conversion.

	private:
	std::string m_codepage;
	std::unique_ptr<UConverter, decltype(&ucnv_close)> m_converter;
	icu::ErrorCode m_error;
	std::mutex m_lock;
};

class UnicodeUtil {
	friend class Songs;
	static std::map<std::string, Converter> m_converters;
	enum class CaseMapping { LOWER, UPPER, TITLE, NONE };

	static std::string getCharset(std::string_view& str);
	static Converter& getConverter(std::string const& s);
	static bool removeUTF8BOM(std::string_view& str);
	
	public:
	UnicodeUtil() = delete;
	~UnicodeUtil() = delete;
	static void collate (songMetadata& stringmap);
	static std::string convertToUTF8 (std::string_view str, std::string _filename = std::string(), CaseMapping toCase = CaseMapping::NONE, bool assumeUTF8 = false);
	static bool caseEqual (std::string_view lhs, std::string_view rhs, bool assumeUTF8 = false);
	static bool isRTL(std::string_view str); ///< FIXME: This won't be used for now, but it might be useful if we eventually implement RTL translations. As-is, at least on my mac, Performous is refusing to render Arabic text, although it might be a font issue.
	static std::string toLower (std::string_view str);
	static std::string toUpper (std::string_view str);
	static std::string toTitle (std::string_view str);

	static std::unique_ptr<icu::RuleBasedCollator> m_searchCollator;
	static std::unique_ptr<icu::RuleBasedCollator> m_sortCollator;
	static std::mutex m_convertersMutex;
};
