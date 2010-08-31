/// @file Some uninteresting helper functions used by ss_extract.

// LibXML2 logging facility
extern "C" void xmlLogger(void* logger, char const* msg, ...) { if (logger) *(std::ostream*)logger << msg; }
void enableXMLLogger(std::ostream& os = std::cerr) { xmlSetGenericErrorFunc(&os, xmlLogger); }
void disableXMLLogger() { xmlSetGenericErrorFunc(NULL, xmlLogger); }


std::string filename(boost::filesystem::path const& p) { return *--p.end(); }

/** Fix Singstar's b0rked XML **/
std::string xmlFix(std::vector<char> const& data) {
	std::string ret;
	for(std::size_t i = 0; i < data.size(); ++i) {
		if (data[i] != '&') ret += data[i]; else ret += "&amp;";
	}
	return ret;
}

/** Erase all occurences of del from str. **/
void safeErase(Glib::ustring& str, Glib::ustring const& del) {
	do {
		Glib::ustring::size_type pos = str.find(del);
		if (pos != Glib::ustring::npos) { str.erase(pos, del.size()); continue; }
	} while (0);
}

/** Fix inconsistencies in SS edition names **/
Glib::ustring prettyEdition(Glib::ustring str) {
	safeErase(str, "®");
	safeErase(str, "™");
	if (str == "SingStar") return "SingStar Original";
	if (str == "SingStar '80s") return "SingStar 80s";
	if (str == "SingStar Schlager") return "SingStar Svenska Hits Schlager";
	if (str == "SingStar Suomi Rock") return "SingStar SuomiRock";
	return str;
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

