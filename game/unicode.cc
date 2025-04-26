#include "unicode.hh"

#include "configuration.hh"
#include "game.hh"
#include "log.hh"

#include <regex>
#include <stdexcept>
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/ubidi.h>
#include "compact_enc_det/compact_enc_det.h"

std::unique_ptr<icu::RuleBasedCollator> UnicodeUtil::m_searchCollator;
std::unique_ptr<icu::RuleBasedCollator> UnicodeUtil::m_sortCollator;

std::map<std::string, Converter> UnicodeUtil::m_converters{};
std::mutex UnicodeUtil::m_convertersMutex;

Converter::Converter(std::string const& codepage): m_codepage(codepage), m_converter(nullptr, &ucnv_close) {
	m_converter = std::unique_ptr<UConverter, decltype(&ucnv_close)>(ucnv_open(m_codepage.c_str(), m_error), &ucnv_close);
	if (m_error.isFailure()) throw std::runtime_error("unicode/error: " + std::to_string(m_error.get()) + ": " + std::string(m_error.errorName()));
}

Converter::Converter(Converter&& c) noexcept:
	m_codepage(std::move(c.m_codepage)),
	m_converter(std::move(c.m_converter)),
	m_error(std::move(c.m_error)) {}

icu::UnicodeString Converter::convertToUTF16(std::string_view sv) {
	std::scoped_lock l(m_lock);
	icu::UnicodeString ret(sv.data(), static_cast<int>(sv.length()), m_converter.get(), m_error);
	if (m_error.isFailure()) throw std::runtime_error("Couldn't convert string: " + std::string(sv) + " to UTF-8. Error: " + std::to_string(m_error.get()) + ": " + m_error.errorName());
	return ret;
}

Converter& UnicodeUtil::getConverter(std::string const& s) {
	std::scoped_lock l{m_convertersMutex};
	return m_converters.try_emplace(s, Converter(s)).first->second;
}

std::string UnicodeUtil::getCharset (std::string_view& str) {
	if (removeUTF8BOM(str)) 
		return "UTF-8";

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
		std::string_view textSample{str.data(), str.size() <= 256 ? str.length() : 255}; // Magic number, so sue me.
		SpdLogger::info(LogSystem::I18N, "Detected encoding={}, for text sample='{}' not reliable.", MimeEncodingName(encoding), textSample); 
	}
	return MimeEncodingName(encoding);
}

std::string UnicodeUtil::convertToUTF8 (std::string_view str, std::string _filename, CaseMapping toCase, bool assumeUTF8) {
	icu::UnicodeString ustring;
	std::string charset;
	if (assumeUTF8) charset = "UTF-8";
	else charset = UnicodeUtil::getCharset(str);
		if (charset != "UTF-8") {
			if (!_filename.empty()) {
				SpdLogger::info(LogSystem::I18N, "Filename={} does not seem to be UTF-8. Detected encoding={}", _filename, charset);
			}
			ustring = UnicodeUtil::getConverter(charset).convertToUTF16(str);
		}
	else {
		if (toCase == CaseMapping::NONE) return std::string{str.data(), str.length()}; // If it's already UTF-8, and we don't need to case-convert, we can just return right away.
		ustring = icu::UnicodeString::fromUTF8(str); // StringPiece can be implicitly converted from a string_view.
	}
	switch(toCase) {
		case CaseMapping::UPPER:
			ustring.toUpper(TranslationEngine::getIcuLocale());
			break;
		case CaseMapping::LOWER:
			ustring.toLower(TranslationEngine::getIcuLocale());
			break;
		case CaseMapping::TITLE:
			ustring.toTitle(0, TranslationEngine::getIcuLocale(), 0);
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
			SpdLogger::error(LogSystem::I18N, "Unable to convert text in unknown encoding={}", charset);
		}
	}
	removeUTF8BOM(ret);
	return ret;
}

bool UnicodeUtil::removeUTF8BOM(std::string_view& str) {
	// Test for UTF-8 BOM (a three-byte sequence at the beginning of a file)
	if (str.substr(0, 3) == "\xEF\xBB\xBF") {
		str = str.substr(3); // Remove BOM if there is one
		return true;
	}
	return false;
}

bool UnicodeUtil::removeUTF8BOM(std::string& str) {
	// Test for UTF-8 BOM (a three-byte sequence at the beginning of a file)
	if (str.substr(0, 3) == "\xEF\xBB\xBF") {
		str = str.substr(3); // Remove BOM if there is one
		return true;
	}
	return false;
}

bool UnicodeUtil::caseEqual (std::string_view lhs, std::string_view rhs, bool assumeUTF8) {
	if (lhs == rhs) return true; // Early return
	std::string lhsCharset = UnicodeUtil::getCharset(lhs);
	std::string rhsCharset = UnicodeUtil::getCharset(rhs);;
	icu::UnicodeString lhsUniString;
	icu::UnicodeString rhsUniString;
	if (lhsCharset != "UTF-8" && !assumeUTF8) {
		lhsUniString = UnicodeUtil::getConverter(lhsCharset).convertToUTF16(lhs); 
	}
	else lhsUniString = icu::UnicodeString::fromUTF8(lhs); // StringPiece can be implicitly converted from a string_view.
	if (rhsCharset != "UTF-8" && !assumeUTF8) {
		rhsUniString = UnicodeUtil::getConverter(rhsCharset).convertToUTF16(rhs);
	}
	else rhsUniString = icu::UnicodeString::fromUTF8(rhs); // StringPiece can be implicitly converted from a string_view.
	int8_t result = lhsUniString.caseCompare(rhsUniString, U_FOLD_CASE_DEFAULT);
	return (result == 0);
}

bool UnicodeUtil::isRTL(std::string_view str) {
	bool _return = false;
	icu::ErrorCode _unicodeError;
	std::string charset(UnicodeUtil::getCharset(str));
	icu::UnicodeString ustring = UnicodeUtil::getConverter(charset).convertToUTF16(str);
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
		ustring.length(),
		UBIDI_DEFAULT_LTR,
		nullptr,
		_unicodeError
	);
	if (_unicodeError.isSuccess()) _return = (ubidi_getDirection(_uBiDiObj.get()) != UBIDI_LTR);
	else {
		SpdLogger::warning(LogSystem::I18N, "Error determining text direction. Will assume LTR. Text='{}', error={}: {}", str, static_cast<int>(_unicodeError.get()), _unicodeError.errorName());
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
