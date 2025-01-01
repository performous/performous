#include "songparserexception.hh"

#include "song.hh"

SongParserException::SongParserException(Song& s, std::string const& msg, unsigned int linenum, bool sil)
: runtime_error(msg), m_filename(s.filename), m_linenum(linenum), m_silent(sil) {
	if (!sil)
		s.b0rked += msg + '\n';
}

std::ostream& operator<<(std::ostream& os, SongParserException const& e) {
	os << (e.silent() ? "songparser/debug: " : "songparser/warning: ") << e.file().string();
	if (e.line()) os << ":" << e.line();
	os << ":\n  " << e.what() << std::endl;
	return os;
}

