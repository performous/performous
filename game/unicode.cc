#include "unicode.hh"

#include "configuration.hh"
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include "../ced/compact_enc_det/compact_enc_det.h"

icu::RuleBasedCollator UnicodeUtil::m_dummyCollator (icu::UnicodeString (""), icu::Collator::PRIMARY, m_staticIcuError);
UErrorCode UnicodeUtil::m_staticIcuError = U_ZERO_ERROR;

MatchResult UnicodeUtil::getCharset (UCharsetDetector* m_chardet, std::string const& str) {
	MatchResult retval;
	const char* text = str.c_str();
	int bytes_consumed;
	bool is_reliable;
	Encoding encoding = CompactEncDet::DetectEncoding(
        text, strlen(text),
        nullptr, nullptr, nullptr,
        UNKNOWN_ENCODING,
        UNKNOWN_LANGUAGE,
        CompactEncDet::WEB_CORPUS,
        true,
        &bytes_consumed,
        &is_reliable);	
// 	std::clog << "unicode/debug: Let's try CED for text: " << text << ", detected: " << EncodingName(encoding) << ", reliable?: " << std::to_string(is_reliable) << std::endl;
	UErrorCode icuError = U_ZERO_ERROR;
	icu::LocalUCharsetDetectorPointer _m_chardet(ucsdet_open(&icuError));
	// 	icu::LocalUCharsetDetectorPointer m_chardet (ucsdet_open (&UnicodeUtil::m_staticIcuError));
	if (U_FAILURE(icuError)) { 
		std::string err = std::string ("unicode/error: Couldn't create LocalUCharsetDetectorPointer: ");
		err += u_errorName(UnicodeUtil::m_staticIcuError);
		throw std::runtime_error (err);
	}
	ucsdet_enableInputFilter(m_chardet, true);
	if (U_FAILURE(icuError)) { 
		std::string err = std::string ("unicode/error: Couldn't enable input filter: ");
		err += u_errorName(UnicodeUtil::m_staticIcuError);
		throw std::runtime_error (err);
	}
	auto string = str.c_str();
	ucsdet_setText(m_chardet, string, strlen(string), &UnicodeUtil::m_staticIcuError);
	if (U_FAILURE(UnicodeUtil::m_staticIcuError)) {
		std::string err = std::string ("unicode/error: Couldn't pass text to CharsetDetector: ");
		err += u_errorName(UnicodeUtil::m_staticIcuError);
		ucsdet_close(m_chardet);
		throw std::runtime_error (err);
	}
	else {
		const UCharsetMatch* match = ucsdet_detect (m_chardet, &UnicodeUtil::m_staticIcuError);
		return std::pair<std::string,int>(ucsdet_getName(match, &UnicodeUtil::m_staticIcuError), ucsdet_getConfidence(match, &UnicodeUtil::m_staticIcuError));
	}	
}

void UnicodeUtil::convertToUTF8 (std::stringstream &_stream, std::string _filename) {
	std::string data = _stream.str();
	MatchResult match;
		UCharsetDetector* m_chardet = ucsdet_open(&m_icuError);
		match = UnicodeUtil::getCharset(m_chardet, data);	
// 		match = std::pair<std::string,int>("UTF-8",100); // If there's no filename, assume it's internal text and thus utf-8.
	icu::UnicodeString ustring;
	// Test for UTF-8 BOM (a three-byte sequence at the beginning of a file)
	if (data.substr(0, 3) == "\xEF\xBB\xBF") {
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
			if (!_filename.empty()) { std::clog << "unicode/info: " << _filename << " does not appear to be UTF-8... (" << charset << ") detected." << std::endl; }
			std::string _str;
			const char* tmp = data.c_str();
			icu::UnicodeString ustring = icu::UnicodeString(tmp, charset.c_str());
			_stream.str(ustring.toUTF8String(_str));
		}
	}
	ucsdet_close(m_chardet);
}

std::string UnicodeUtil::convertToUTF8 (std::string const& str) {
	std::stringstream ss (str);
	convertToUTF8 (ss, std::string());
	return ss.str();
}

std::string unicodeCollate (std::string const& str) {
	ConfigItem::StringList termsToCollate = config["game/sorting_ignore"].sl();
	std::string pattern = std::string ("^((");
	for (auto term : termsToCollate) {
		if (term != termsToCollate.front()) {
			pattern += std::string("|");
		}
		pattern += term;
		if (term == termsToCollate.back()) {
			pattern += std::string(")\\s(.+))$");
		}
	}
	std::string collated = std::regex_replace(UnicodeUtil().convertToUTF8(str), std::regex(pattern, std::regex_constants::icase), "\\3 \\2");
	return collated;
}
