/// @file Some uninteresting helper functions used by ss_extract.

#include <boost/algorithm/string.hpp>
#include <libxml++/libxml++.h>
#include <glibmm/convert.h>
#include "pak.h"

// LibXML2 logging facility
extern "C" void xmlLogger(void* logger, char const* msg, ...) { if (logger) *(std::ostream*)logger << msg; }
void enableXMLLogger(std::ostream& os = std::cerr) { xmlSetGenericErrorFunc(&os, xmlLogger); }
void disableXMLLogger() { xmlSetGenericErrorFunc(NULL, xmlLogger); }

#if BOOST_FILESYSTEM_VERSION < 3
std::string filename(boost::filesystem::path const& p) { return *--p.end(); }
#else
std::string filename(boost::filesystem::path const& p) { return p.filename().string(); }
#endif

namespace xmlpp {
	// typedef Node::const_NodeSet const_NodeSet; // correct libxml++ 3.0 implementation
	typedef NodeSet const_NodeSet; // implementation to satisfy libxml++ 2.6 API

	static inline const TextNode* get_first_child_text(const Element& element) {
		return element.get_child_text();
	}
}

/** Fix Singstar's b0rked XML **/
std::string xmlFix(std::vector<char> const& data) {
	std::string ret;
	for(std::size_t i = 0; i < data.size(); ++i) {
		if (data[i] != '&') ret += data[i]; else ret += "&amp;";
	}
	return ret;
}

struct SSDom: public xmlpp::DomParser {
	xmlpp::Node::PrefixNsMap nsmap;
	SSDom(PakFile const& file) {
		std::vector<char> tmp;
		file.get(tmp);
		std::string buf = xmlFix(tmp);
		load(buf);
	}
	SSDom() {}
	void load(std::string const& buf) {
		set_substitute_entities();
		try {
			struct DisableLogger {
				DisableLogger() { disableXMLLogger(); }
				~DisableLogger() { enableXMLLogger(); }
			} disabler;
			parse_memory(buf);
		} catch (...) {
			std::string buf2 = Glib::convert(buf, "UTF-8", "ISO-8859-1"); // Convert to UTF-8
			parse_memory(buf2);
		}
		nsmap["ss"] = get_document()->get_root_node()->get_namespace_uri();
	}
	bool find(xmlpp::Element& elem, std::string xpath, xmlpp::const_NodeSet& n) {
		if (nsmap["ss"].empty()) boost::erase_all(xpath, "ss:");
		n = elem.find(xpath, nsmap);
		return !n.empty();
	}
	bool find(std::string const& xpath, xmlpp::const_NodeSet& n) {
		return find(*get_document()->get_root_node(), xpath, n);
	}
	bool getValue(std::string const& xpath, std::string& result) {
		xmlpp::const_NodeSet n;
		if (!find(xpath, n)) return false;
		result = xmlpp::get_first_child_text(dynamic_cast<xmlpp::Element&>(*n[0]))->get_content();
		return true;
	}
};

/** Erase all occurences of del from str. **/
void safeErase(Glib::ustring& str, Glib::ustring const& del) {
	do {
		Glib::ustring::size_type pos = str.find(del);
		if (pos != Glib::ustring::npos) { str.erase(pos, del.size()); continue; }
	} while (0);
}

/** Normalize whitespace **/
Glib::ustring normalize(Glib::ustring const& str) {
	Glib::ustring ret;
	bool first = true;
	bool ws = true;
	for (Glib::ustring::const_iterator it = str.begin(); it != str.end(); ++it) {
		if (std::isspace(*it)) { ws = true; continue; }
		if (first) {
			first = false;
			ws = false;
			ret = Glib::ustring(1, *it).uppercase();
			continue;
		}
		if (ws) { ws = false; ret += ' '; }
		ret += *it;
	}
	return ret;
}

/** Sanitize a string into a form that can be safely used as a filename. **/
std::string safename(Glib::ustring const& str) {
	Glib::ustring ret;
	Glib::ustring forbidden("\"*/:;<>?\\^`|~");
	for (Glib::ustring::const_iterator it = str.begin(); it != str.end(); ++it) {
		bool first = it == str.begin();
		if (*it < 0x20) continue; // Control characters
		if (*it >= 0x7F && *it < 0xA0) continue; // Additional control characters
		if (first && *it == '.') continue;
		if (first && *it == '-') continue;
		if (*it == '&') { ret += " and "; continue; }
		if (*it == '%') { ret += " percent "; continue; }
		if (*it == '$') { ret += " dollar "; continue; }
		if (forbidden.find(*it) != Glib::ustring::npos) { ret += "_"; continue; }
		ret += *it;
	}
	return normalize(ret);
}

/** Fix inconsistencies in SS edition names **/
Glib::ustring prettyEdition(Glib::ustring str) {
	str = normalize(str);
	safeErase(str, "®");
	safeErase(str, "™");
	if (str == "SingStar") return "SingStar Original";
	if (str == "SingStar '80s") return "SingStar 80s";
	if (str == "SingStar Schlager") return "SingStar Svenska Hits Schlager";
	if (str == "SingStar Suomi Rock") return "SingStar SuomiRock";
	return str;
}

