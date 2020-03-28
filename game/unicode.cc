#include "unicode.hh"

#include "configuration.hh"
#include "regex.hh"
#include <sstream>
#include <stdexcept>
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include "../ced/compact_enc_det/compact_enc_det.h"

UErrorCode UnicodeUtil::m_staticIcuError = UErrorCode::U_ZERO_ERROR;
icu::RuleBasedCollator UnicodeUtil::m_dummyCollator (icu::UnicodeString (""), icu::Collator::PRIMARY, m_staticIcuError);
icu::RuleBasedCollator UnicodeUtil::m_sortCollator  (nullptr, icu::Collator::SECONDARY, m_staticIcuError);

std::string UnicodeUtil::getCharset (std::string const& str) {
	const char* text = str.c_str();
	int bytes_consumed;
	bool is_reliable;
	
	Encoding encoding = CompactEncDet::DetectEncoding(
        text, strlen(text),
        nullptr, nullptr, nullptr,
        Encoding::UNKNOWN_ENCODING,
        Language::UNKNOWN_LANGUAGE,
        CompactEncDet::WEB_CORPUS,
        true,
        &bytes_consumed,
        &is_reliable);
        
	if (!is_reliable) {
			std::clog << "unicode/warning: detected encoding (" <<
			MimeEncodingName(encoding) << ") for text: " <<
			((str.size() <= 256) ? str : str.substr(0,255)) <<
			" was flagged as not reliable." <<
			std::endl; // Magic number, so sue me.
		}
	return MimeEncodingName(encoding);
}

void UnicodeUtil::convertToUTF8 (std::stringstream &_stream, std::string _filename) {
	std::string data = _stream.str();
	std::string charset = UnicodeUtil::getCharset(data);	
	icu::UnicodeString ustring;
	// Test for UTF-8 BOM (a three-byte sequence at the beginning of a file)
	if (data.substr(0, 3) == "\xEF\xBB\xBF") {
		_stream.str(data.substr(3)); // Remove BOM if there is one
	}
	if (charset != "UTF-8") {
		if (!_filename.empty()) { std::clog << "unicode/info: " << _filename << " does not appear to be UTF-8; (" << charset << ") detected." << std::endl; }
		std::string _str;
		const char* tmp = data.c_str();
		icu::UnicodeString ustring = icu::UnicodeString(tmp, charset.c_str());
		if (!ustring.isEmpty()) {
			_stream.str(ustring.toUTF8String(_str));
		}
		else {
			if (!data.empty()) {
				std::clog << "unicode/error: tried to convert text in an unknown encoding: " << charset << std::endl;
			}
		}
	}
}

std::string UnicodeUtil::convertToUTF8 (std::string const& str) {
	std::stringstream ss (str);
	convertToUTF8 (ss, std::string());
	return ss.str();
}

std::string UnicodeUtil::toLower (std::string const& str, size_t length) {
	std::stringstream ss (str);
	convertToUTF8 (ss, std::string());
	std::string ret;
	if (length == 0 || length >= str.size()) length = str.size();
		icu::UnicodeString tmp = icu::UnicodeString::fromUTF8(ss.str());
		tmp = tmp.tempSubString(0, length).toLower() + tmp.tempSubString(length, tmp.length() - 1);
		tmp.toUTF8String(ret);
	return ret;
}

std::string UnicodeUtil::toUpper (std::string const& str, size_t length) {
	std::stringstream ss (str);
	convertToUTF8 (ss, std::string());
	std::string ret;
	if (length == 0 || length >= str.size()) length = str.size();
		icu::UnicodeString tmp = icu::UnicodeString::fromUTF8(ss.str());
		tmp = tmp.tempSubString(0, length).toUpper() + tmp.tempSubString(length, tmp.length() - 1);
		tmp.toUTF8String(ret);
	return ret;
}

void UnicodeUtil::collate (songMetadata& stringmap) {
	for (auto& kv: stringmap) { 
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
		std::string collatedString = regex_replace(convertToUTF8(kv.second),
		regex(pattern, regex_constants::icase), "$3,$2");
		stringmap[kv.first] = collatedString;
	}
}
