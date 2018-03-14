#pragma once

#include <iostream>
#include <string>
#include <unicode/tblcoll.h>
#include <unicode/ucsdet.h>



void convertToUTF8(std::stringstream &_stream, std::string _filename);
std::string convertToUTF8(std::string const& str);
std::string unicodeCollate(std::string const& str);

typedef std::pair<const char*, int> MatchResult;

struct UnicodeUtil {

UnicodeUtil() {};
~UnicodeUtil() {};
friend class Songs;

static MatchResult getCharset(std::string const& str);

private:
static UErrorCode m_icuError;
static icu::RuleBasedCollator m_dummyCollator;
};