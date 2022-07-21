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
icu::RuleBasedCollator UnicodeUtil::m_searchCollator (icu::UnicodeString (""), icu::Collator::PRIMARY, m_staticIcuError);
icu::RuleBasedCollator UnicodeUtil::m_sortCollator  (nullptr, icu::Collator::SECONDARY, m_staticIcuError);

std::map<std::string, Converter> UnicodeUtil::m_converters{};

Converter::Converter(std::string const& codepage): m_codepage(codepage), m_converter(nullptr, &ucnv_close) {
	m_converter = std::unique_ptr<UConverter, decltype(&ucnv_close)>(ucnv_open(m_codepage.c_str(), m_error), &ucnv_close);
	if (m_error.isFailure()) throw std::runtime_error("unicode/error: " + std::to_string(m_error.get()) + ": " + std::string(m_error.errorName()));
}

Converter::Converter(Converter&& c) noexcept: m_codepage(std::move(c.m_codepage)), m_converter(std::move(c.m_converter)), m_error(std::move(c.m_error)) {}

void Converter::reset() {
	std::scoped_lock l(m_lock);
	if (m_converter) ucnv_reset(m_converter.get());
}

icu::UnicodeString Converter::convertToUTF8(std::string_view sv) {
	std::scoped_lock l(m_lock);
	icu::UnicodeString ret(sv.data(), static_cast<int>(sv.length()), m_converter.get(), m_error);
	if (m_error.isFailure()) throw std::runtime_error("Couldn't convert string: " + std::string(sv) + " to UTF-8. Error: " + std::to_string(m_error.get()) + ": " + m_error.errorName());
	return ret;
}

Converter& UnicodeUtil::getConverter(std::string const& s) {
	return m_converters.try_emplace(s, Converter(s)).first->second;
}

std::string UnicodeUtil::getCharset (std::string_view str) {
	int bytes_consumed;
	bool is_reliable;
	
	Encoding encoding = CompactEncDet::DetectEncoding(
		str.data(), static_cast<int>(str.size()),
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

std::string UnicodeUtil::convertToUTF8 (std::string_view str, std::string _filename, CaseMapping toCase, bool assumeUTF8) {
	icu::UnicodeString ustring;
	std::string charset;
	if (assumeUTF8) ustring = icu::UnicodeString::fromUTF8(str.data());
	else {
		if (removeUTF8BOM(str)) charset = "UTF-8";
		else { charset = UnicodeUtil::getCharset(str); }
		}
		if (charset != "UTF-8") {
			if (!_filename.empty()) std::clog << "unicode/info: " << _filename << " does not appear to be UTF-8; (" << charset << ") detected." << std::endl; 
			ustring = UnicodeUtil::getConverter(charset).convertToUTF8(str.data());
		}
	else { ustring = icu::UnicodeString::fromUTF8(str.data()); }
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
	std::string ret;
	if (!ustring.isEmpty()) {
		ustring.toUTF8String(ret);
	}
	else {
		if (!ret.empty()) {
			std::clog << "unicode/error: tried to convert text in an unknown encoding: " << charset << std::endl;
		}
	}
	return ret;
}

bool UnicodeUtil::removeUTF8BOM(std::string_view str) {
	// Test for UTF-8 BOM (a three-byte sequence at the beginning of a file)
	if (str.substr(0, 3) == "\xEF\xBB\xBF") {
		str = str.substr(3); // Remove BOM if there is one
		return true;
	}
	return false;
}

bool UnicodeUtil::caseEqual (std::string_view lhs, std::string_view rhs, bool assumeUTF8) {
	if (lhs == rhs) return true; // Early return
	std::string lhsCharset;
	std::string rhsCharset;
	icu::UnicodeString lhsUniString;
	icu::UnicodeString rhsUniString;
	if (removeUTF8BOM(lhs)) lhsCharset = "UTF-8";
	if (removeUTF8BOM(rhs)) rhsCharset = "UTF-8";
	if (lhsCharset != "UTF-8" && !assumeUTF8) {
		lhsCharset = UnicodeUtil::getCharset(lhs);
		lhsUniString = UnicodeUtil::getConverter(lhsCharset).convertToUTF8(lhs.data());
	}
	else lhsUniString = icu::UnicodeString::fromUTF8(lhs.data());
	if (rhsCharset != "UTF-8" && !assumeUTF8) {
		rhsCharset = UnicodeUtil::getCharset(rhs);
		rhsUniString = UnicodeUtil::getConverter(rhsCharset).convertToUTF8(rhs.data());
	}
	else rhsUniString = icu::UnicodeString::fromUTF8(rhs.data());
	int8_t result = lhsUniString.caseCompare(rhsUniString, U_FOLD_CASE_DEFAULT);
	return (result == 0);
}

bool UnicodeUtil::isRTL(std::string_view str) {
	bool _return = false;
	icu::ErrorCode _unicodeError;
	std::string charset(UnicodeUtil::getCharset(str));
	icu::UnicodeString ustring = UnicodeUtil::getConverter(charset).convertToUTF8(str.data());
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

std::string UnicodeUtil::toLower (std::string_view str) {
	return convertToUTF8 (str, "", CaseMapping::LOWER);
}

std::string UnicodeUtil::toUpper (std::string_view str) {
	return convertToUTF8 (str, "", CaseMapping::UPPER);
}

std::string UnicodeUtil::toTitle (std::string_view str) {
	return convertToUTF8 (str, "", CaseMapping::TITLE);
}

void UnicodeUtil::collate (songMetadata& stringmap) {
	for (auto const& [key, value]: stringmap) { 
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
