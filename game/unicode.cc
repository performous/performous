#include "unicode.hh"

#include "configuration.hh"
#include "game.hh"

#include <regex>
#include <stdexcept>
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/ubidi.h>
#include "compact_enc_det/compact_enc_det.h"

icu::ErrorCode UnicodeUtil::m_staticIcuError = icu::ErrorCode();
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
	if (assumeUTF8) ustring = icu::UnicodeString::fromUTF8(str);
	else {
		if (removeUTF8BOM(ret)) charset = "UTF-8";
		else { charset = UnicodeUtil::getCharset(str); }
		}
		if (charset != "UTF-8") {
			if (!_filename.empty()) std::clog << "unicode/info: " << _filename << " does not appear to be UTF-8; (" << charset << ") detected." << std::endl; 
			ustring = icu::UnicodeString(ret.c_str(), charset.c_str());
		}
	else { ustring = icu::UnicodeString::fromUTF8(ret); }
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

bool UnicodeUtil::removeUTF8BOM(std::string& str) {
	// Test for UTF-8 BOM (a three-byte sequence at the beginning of a file)
	if (str.substr(0, 3) == "\xEF\xBB\xBF") {
		str = str.substr(3); // Remove BOM if there is one
		return true;
	}
	return false;
}

bool UnicodeUtil::caseEqual (std::string const& lhs, std::string const& rhs, bool assumeUTF8) {
	if (lhs == rhs) return true; // Early return
	
	std::string a = lhs;
	std::string b = rhs;
	std::string aCharset;
	std::string bCharset;
	icu::UnicodeString aUniString;
	icu::UnicodeString bUniString;
	if (removeUTF8BOM(a)) aCharset = "UTF-8";
	if (removeUTF8BOM(b)) bCharset = "UTF-8";
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

bool UnicodeUtil::isRTL(std::string const& str) {
	bool _return = false;
	icu::ErrorCode _unicodeError;
	std::string charset = UnicodeUtil::getCharset(str);
	icu::UnicodeString ustring = icu::UnicodeString(str.c_str(), charset.c_str());
	 std::unique_ptr<UBiDi,void(*)(UBiDi*)> _uBiDiObj(
		ubidi_open(),
		[](UBiDi* p) {
			if (p != nullptr) {
			ubidi_close(p);
			}
		});
	ubidi_setPara(
		_uBiDiObj.get(),
		ustring.getBuffer(),
		-1,
		UBIDI_DEFAULT_LTR,
		nullptr,
		_unicodeError
	);
	if (_unicodeError.isSuccess()) _return = (ubidi_getDirection(_uBiDiObj.get()) != UBIDI_LTR);
	else {
		std::clog << "unicode/warning: Error (" << std::to_string(_unicodeError.get()) << ": " << _unicodeError.errorName() << "), determining text direction for: " << str << ", will assume LTR." << std::endl;
		}
	return _return;
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
