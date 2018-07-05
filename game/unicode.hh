#pragma once

#include <iostream>
#include <string>
#include <unicode/tblcoll.h>
#include <unicode/uclean.h>
#include <unicode/ucsdet.h>
#include <unicode/utypes.h>

// void convertToUTF8(std::stringstream &_stream, std::string _filename);
// std::string convertToUTF8(std::string const& str);
std::string unicodeCollate(std::string const& str);

typedef std::pair<std::string, int> MatchResult;

struct UnicodeUtil {
	UnicodeUtil(): m_icuError(U_ZERO_ERROR) { 
		u_init(&m_icuError);
		if (U_FAILURE(m_icuError)) {
			std::string err = std::string ("unicode/error: Couldn't initialize ICU: ");
			err += u_errorName(m_icuError);
			throw std::runtime_error (err);
		}
// 		}

	};
	~UnicodeUtil() {};
	friend class Songs;
	static MatchResult getCharset(UCharsetDetector* m_chardet, std::string const& str);
	void convertToUTF8 (std::stringstream &_stream, std::string _filename);
	std::string convertToUTF8 (std::string const& str);
  private:
	UErrorCode m_icuError;
	static UErrorCode m_staticIcuError;
	static icu::RuleBasedCollator m_dummyCollator;
};
