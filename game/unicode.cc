#include "unicode.hh"
#include "configuration.hh"

#include <regex>
#include <boost/scoped_ptr.hpp>
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <sstream>
#include <stdexcept>

UErrorCode UnicodeUtil::m_icuError = U_ZERO_ERROR;
icu::RuleBasedCollator UnicodeUtil::m_dummyCollator(icu::UnicodeString(""), icu::Collator::PRIMARY, m_icuError);

MatchResult UnicodeUtil::getCharset (std::string const& str) {
    MatchResult retval;
    LocalUCharsetDetectorPointer m_chardet(ucsdet_open(&UnicodeUtil::m_icuError));
    ucsdet_enableInputFilter(m_chardet.getAlias(), true); 
    auto string = str.c_str();
    ucsdet_setText(m_chardet.getAlias(), string, -1, &m_icuError);
    if (U_FAILURE(UnicodeUtil::m_icuError)) {
        std::string err = std::string("unicode/error: Couldn't pass text to CharsetDetector: ");
        err += u_errorName(m_icuError);
        throw std::runtime_error(err);
    }
    else {
        const UCharsetMatch* match = ucsdet_detect(m_chardet.getAlias(), &m_icuError);
        return std::pair<std::string,int>(ucsdet_getName(match, &m_icuError), ucsdet_getConfidence(match, &m_icuError));
    }
}

void convertToUTF8(std::stringstream &_stream, std::string _filename) {
    std::string data = _stream.str();
    MatchResult match;
    if (!_filename.empty()) {
        match = UnicodeUtil::getCharset(data);
    }
    else {
        match = std::pair<std::string,int>("UTF-8",100); // If there's no filename, assume it's internal text and thus utf-8.
    }
    icu::UnicodeString ustring;
    if (data.substr(0, 3) == "\xEF\xBB\xBF") {
        if (config["game/bom_warnings"].b()) {
            std::clog << "unicode/warning: " << _filename << " UTF-8 BOM ignored. Please avoid editors that add a BOM to UTF-8 (e.g. Notepad)." << std::endl;
        }
        match.first = "UTF-8";
        match.second = 100;
        _stream.str(data.substr(3)); // Remove BOM if there is one
    }
    if (match.second > 10 && match.second < 50) { // 50 is a really good match, 10 means an encoding that could be conceivably used to display the text.
        if (match.first == "ISO-8859-1" || match.first == "ISO-8859-2") {
            match.first = "UTF-8";
            match.second = 75;	// Mostly western characters. Let's treat it as a UTF-8 false-negative.
        }
    }
    if (match.second >= 50) { // fairly good match?
        std::string charset = match.first;
        if (charset != "UTF-8") {
            if (!_filename.empty()) { std::clog << "unicode/warning: " << _filename << " does not appear to be UTF-8... (" << charset << ") detected." << std::endl; }
            std::string _str;
            const char* tmp = data.c_str();
            icu::UnicodeString ustring = icu::UnicodeString(tmp, charset.c_str());
            _stream.str(ustring.toUTF8String(_str));
        }
    }
}

std::string convertToUTF8(std::string const& str) {
    std::stringstream ss(str);
    convertToUTF8(ss, std::string());
    return ss.str();
}

std::string unicodeCollate(std::string const& str) {
	ConfigItem::StringList termsToCollate = config["game/sorting_ignore"].sl();
	std::string pattern = std::string("^((");
	for (auto term: termsToCollate) {
		if (term != termsToCollate.front()) { pattern += std::string("|"); }
        	pattern += term;
        	if (term == termsToCollate.back()) { pattern += std::string(")\\s(.+))$"); }
	}	
	std::string collated = std::regex_replace(convertToUTF8(str), std::regex(pattern, std::regex_constants::icase), "\\3 \\2");
	return collated;
}
