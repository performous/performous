#include "unicode.hh"

#include "configuration.hh"
#include "game.hh"

#include <regex>
#include <sstream>
#include <stdexcept>
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include "compact_enc_det/compact_enc_det.h"

UErrorCode UnicodeUtil::m_staticIcuError = U_ZERO_ERROR;
icu::RuleBasedCollator UnicodeUtil::m_dummyCollator (icu::UnicodeString (""), icu::Collator::PRIMARY, m_staticIcuError);
icu::RuleBasedCollator UnicodeUtil::m_sortCollator  (nullptr, icu::Collator::SECONDARY, m_staticIcuError);

std::string UnicodeUtil::getCharset (std::string const& str) {
	int bytes_consumed;
	bool is_reliable;
	
	Encoding encoding = CompactEncDet::DetectEncoding(
        str.c_str(), static_cast<int>(str.size()),
        nullptr, nullptr, nullptr,
        UNKNOWN_ENCODING,
        UNKNOWN_LANGUAGE,
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

std::string UnicodeUtil::convertToUTF8 (std::string const& str, std::string _filename, CaseMapping toCase, bool assumeUTF8) {
	std::string ret = str;
	icu::UnicodeString ustring;
	std::string charset;
	if (assumeUTF8) {
		ustring = icu::UnicodeString::fromUTF8(str);
	}
	else {
		if (ret.substr(0, 3) == "\xEF\xBB\xBF") {
			ret = ret.substr(3); // Remove BOM if there is one
			charset = "UTF-8";
		}
		else { charset = UnicodeUtil::getCharset(str); }
		// Test for UTF-8 BOM (a three-byte sequence at the beginning of a file)
		if (charset != "UTF-8") {
			if (!_filename.empty()) { std::clog << "unicode/info: " << _filename << " does not appear to be UTF-8; (" << charset << ") detected." << std::endl; }
			ustring = icu::UnicodeString(ret.c_str(), charset.c_str());
		}
		else { ustring = icu::UnicodeString::fromUTF8(ret); }
	}
	switch(toCase) {
		case CaseMapping::UPPER:
			ustring.toUpper();
			break;
		case CaseMapping::LOWER:
			ustring.toLower();
			break;
		case CaseMapping::TITLE:
			ustring.toTitle(0, icu::Locale(Game::getSingletonPtr()->getCurrentLanguageCode().c_str()), U_TITLECASE_NO_LOWERCASE);
			break;
		case CaseMapping::NONE:
			break;
	}
	if (!ustring.isEmpty()) {
		ret.clear();
		ustring.toUTF8String(ret);
	}
	else {
		if (!ret.empty()) {
			std::clog << "unicode/error: tried to convert text in an unknown encoding: " << charset << std::endl;
		}
	}
	return ret;
}

bool UnicodeUtil::caseEqual (std::string const& lhs, std::string const& rhs, bool assumeUTF8) {
	if (lhs == rhs) return true; // Early return
	
	std::string a = lhs;
	std::string b = rhs;
	std::string aCharset;
	std::string bCharset;
	icu::UnicodeString aUniString;
	icu::UnicodeString bUniString;
	if (a.substr(0, 3) == "\xEF\xBB\xBF") {
			a = a.substr(3); // Remove BOM if there is one
			aCharset = "UTF-8";
	}
	if (b.substr(0, 3) == "\xEF\xBB\xBF") {
			b = b.substr(3); // Remove BOM if there is one
			bCharset = "UTF-8";
	}
	if (aCharset != "UTF-8" && !assumeUTF8) {
		std::string aCharset = UnicodeUtil::getCharset(a);
		aUniString = icu::UnicodeString(a.c_str(), aCharset.c_str());
	}
	else aUniString = icu::UnicodeString::fromUTF8(a);
	if (bCharset != "UTF-8" && !assumeUTF8) {
		std::string bCharset = UnicodeUtil::getCharset(b);
		bUniString = icu::UnicodeString(b.c_str(), bCharset.c_str());
	}
	else bUniString = icu::UnicodeString::fromUTF8(b);
	int8_t result = aUniString.caseCompare(bUniString, U_FOLD_CASE_DEFAULT);
	return (result == 0);
}

std::string UnicodeUtil::toLower (std::string const& str) {
	return convertToUTF8 (str, "", CaseMapping::LOWER);
}

std::string UnicodeUtil::toUpper (std::string const& str) {
	return convertToUTF8 (str, "", CaseMapping::UPPER);
}

std::string UnicodeUtil::toTitle (std::string const& str) {
	return convertToUTF8 (str, "", CaseMapping::TITLE);
}

void UnicodeUtil::collate (songMetadata& stringmap) {
	for (auto& [key, value]: stringmap) { 
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
		std::string collatedString = regex_replace(convertToUTF8(value),
		std::regex(pattern, std::regex_constants::icase), "$3,$2");
		stringmap[key] = collatedString;
	}
}
