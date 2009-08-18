#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>
#include <glibmm/convert.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libxml++/libxml++.h>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "pak.h"
#include "ipuconv.hh"
#include "ss_cover.hh"

namespace fs = boost::filesystem;

std::string dvdPath;
std::ofstream txtfile;
int ts = 0;
int sleepts = -1;
xmlpp::Node::PrefixNsMap nsmap;
std::string ns;
const bool video = true;
const bool mkvcompress = true;
const bool oggcompress = true;

// LibXML2 logging facility
extern "C" void xmlLogger(void* logger, char const* msg, ...) { if (logger) *(std::ostream*)logger << msg; }
void enableXMLLogger(std::ostream& os = std::cerr) { xmlSetGenericErrorFunc(&os, xmlLogger); }
void disableXMLLogger() { xmlSetGenericErrorFunc(NULL, xmlLogger); }

void safeErase(Glib::ustring& str, Glib::ustring const& del) {
	do {
		Glib::ustring::size_type pos = str.find(del);
		if (pos != Glib::ustring::npos) { str.erase(pos, del.size()); continue; }
	} while (0);
}

Glib::ustring prettyEdition(Glib::ustring str) {
	safeErase(str, "®");
	safeErase(str, "™");
	if (str == "SingStar") return "SingStar Original";
	if (str == "SingStar '80s") return "SingStar 80s";
	if (str == "SingStar Schlager") return "SingStar Svenska Hits Schlager";
	if (str == "SingStar Suomi Rock") return "SingStar SuomiRock";
	return str;
}

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

/** Automatic conversion helper, not to be used directly. **/
struct Filename_: public std::string {
	Filename_(Glib::ustring const& str_): std::string(str_) {}
	operator char const*() { return c_str(); }
};

/** Sanitize a string into a form that can be safely used as a filename. **/
Filename_ safename(Glib::ustring const& str) {
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
	return Filename_(normalize(ret));
}

void parseNote(xmlpp::Node* node) {
	xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(*node);
	char type = ':';
	std::string lyric = elem.get_attribute("Lyric")->get_value();
	// Some extra formatting to make lyrics look better (hyphen removal & whitespace)
	if (lyric.size() > 0 && lyric[lyric.size() - 1] == '-') {
		if (lyric.size() > 1 && lyric[lyric.size() - 2] == ' ') lyric.erase(lyric.size() - 2);
		else lyric[lyric.size() - 1] = '~';
	} else {
		lyric += ' ';
	}
	int note = std::atoi(elem.get_attribute("MidiNote")->get_value().c_str());
	int duration = std::atoi(elem.get_attribute("Duration")->get_value().c_str());
	if (elem.get_attribute("FreeStyle")) type = 'F';
	if (elem.get_attribute("Bonus")) type = '*';
	if (note) {
		if (sleepts > 0) txtfile << "- " << sleepts << '\n';
		sleepts = 0;
		txtfile << type << ' ' << ts << ' ' << duration << ' ' << note << ' ' << lyric << '\n';
	}
	ts += duration;
}

void parseSentence(xmlpp::Node* node) {
	xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(*node);
	xmlpp::NodeSet n = elem.find(ns + "NOTE", nsmap);
	if (sleepts != -1) sleepts = ts;
	std::for_each(n.begin(), n.end(), parseNote);
}

#include "adpcm.h"

unsigned getLE16(char* buf) { unsigned char* b = reinterpret_cast<unsigned char*>(buf); return b[0] | (b[1] << 8); }

struct Song {
	std::string dataPakName, title, artist, genre, edition, year;
	fs::path path, music, vocals, video, background, cover;
	unsigned samplerate;
	double tempo;
	Song(): samplerate(), tempo() {}
};

void writeWavHeader(std::ostream& outfile, unsigned ch, unsigned sr, unsigned samples) {
	unsigned bps = ch * 2; // Bytes per sample
	unsigned datasize = bps * samples;
	unsigned size = datasize + 0x2C;
	outfile.write("RIFF" ,4); // RIFF chunk
	{ unsigned int tmp=size-0x8 ; outfile.write((char*)(&tmp),4); } // RIFF chunk size
	outfile.write("WAVEfmt ",8); // WAVEfmt header
	{ int   tmp=0x00000010 ; outfile.write((char*)(&tmp),4); } // Always 0x10
	{ short tmp=0x0001     ; outfile.write((char*)(&tmp),2); } // Always 1
	{ short tmp = ch; outfile.write((char*)(&tmp),2); } // Number of channels
	{ int   tmp = sr; outfile.write((char*)(&tmp),4); } // Sample rate
	{ int   tmp = bps * sr; outfile.write((char*)(&tmp),4); } // Bytes per second
	{ short tmp = bps; outfile.write((char*)(&tmp),2); } // Bytes per frame
	{ short tmp = 16; outfile.write((char*)(&tmp),2); } // Bits per sample
	outfile.write("data",4); // data chunk
	{ int   tmp = datasize; outfile.write((char*)(&tmp),4); }
}

void writeMusic(fs::path const& filename, std::vector<short> const& buf, unsigned sr) {
	std::ofstream f(filename.string().c_str(), std::ios::binary);
	writeWavHeader(f, 2, sr, buf.size());
	f.write(reinterpret_cast<char const*>(&buf[0]), buf.size() * sizeof(short));
}

void music(Song& song, PakFile const& dataFile, PakFile const& headerFile, fs::path const& outPath) {
	std::vector<char> data;
	headerFile.get(data);
	unsigned sr = getLE16(&data[12]);
	unsigned interleave = getLE16(&data[16]);
	const unsigned decodeChannels = 4; // Do not change!
	Adpcm adpcm(interleave, decodeChannels);
	std::vector<short> pcm[2];
	bool karaoke = false;
	for (unsigned pos = 0, end; (end = pos + 2 * adpcm.chunkBytes()) <= dataFile.size; pos = end) {
		dataFile.get(data, pos, end - pos);
		std::vector<short> pcmtmp(adpcm.chunkFrames() * decodeChannels);
		adpcm.decodeChunk(&data[0], pcmtmp.begin());
		for (size_t s = 0; s < pcmtmp.size(); s += 4) {
			short l1 = pcmtmp[s];
			short r1 = pcmtmp[s + 1];
			short l2 = pcmtmp[s + 2];
			short r2 = pcmtmp[s + 3];
			pcm[0].push_back(l1);
			pcm[0].push_back(r1);
			pcm[1].push_back(l2);
			pcm[1].push_back(r2);
			if (l2 != 0 || r2 != 0) karaoke = true;
		}
	}
	std::string ext;
	writeMusic(song.music = outPath / ("music.wav"), pcm[0], sr);
	if (karaoke) writeMusic(song.vocals = outPath / ("vocals.wav"), pcm[1], sr);
	if( oggcompress ) {
		song.music = outPath / ("music.ogg");
		if (karaoke) song.vocals = outPath / ("vocals.ogg");
	}
}

struct Match {
	std::string left, right;
	Match(std::string l, std::string r): left(l), right(r) {}
	bool operator()(Pak::files_t::value_type const& f) {
		std::string n = f.first;
		return n.substr(0, left.size()) == left && n.substr(n.size() - right.size()) == right;
	}
};

/** Fix Singstar's b0rked XML **/
std::string xmlFix(std::vector<char> const& data) {
	std::string ret;
	for(std::size_t i = 0; i < data.size(); ++i) {
		if (data[i] != '&') ret += data[i]; else ret += "&amp;";
	}
	return ret;
}

std::string filename(boost::filesystem::path const& p) { return *--p.end(); }

void saveTxtFile(xmlpp::NodeSet &sentence, const fs::path &path, const Song &song, const std::string singer = "") {
	fs::path file_path;

	if( singer.empty() ) {
		file_path = path / "notes.txt";
	} else {
		file_path = path;
		file_path /= safename(std::string("notes") + " (" + singer + ")" + ".txt");
	}
	txtfile.open(file_path.string().c_str());
	if( singer.empty() )
		txtfile << "#TITLE:" << song.title << std::endl;
	else
		txtfile << "#TITLE:" << song.title << " (" << singer << ")" << std::endl;
	txtfile << "#ARTIST:" << song.artist << std::endl;
	if (!song.genre.empty()) txtfile << "#GENRE:" << song.genre << std::endl;
	if (!song.year.empty()) txtfile << "#YEAR:" << song.year << std::endl;
	if (!song.edition.empty()) txtfile << "#EDITION:" << song.edition << std::endl;
	//txtfile << "#LANGUAGE:English" << std::endl; // Detect instead of hardcoding? 
	if (!song.music.empty()) txtfile << "#MP3:" << filename(song.music) << std::endl;
	if (!song.vocals.empty()) txtfile << "#VOCALS:" << filename(song.vocals) << std::endl;
	if (video && mkvcompress) {
		txtfile << "#VIDEO:video.m4v" << std::endl;
	} else {
		txtfile << "#VIDEO:video.mpg" << std::endl;
	}
	txtfile << "#COVER:cover.jpg" << std::endl;
	//txtfile << "#BACKGROUND:background.jpg" << std::endl;
	txtfile << "#BPM:" << song.tempo << std::endl;
	ts = 0;
	sleepts = -1;
	std::for_each(sentence.begin(), sentence.end(), parseSentence);
	txtfile << 'E' << std::endl;
	txtfile.close();
}

struct Process {
	Pak const& pak;
	Process(Pak const& p): pak(p) {}
	void operator()(std::pair<std::string const, Song>& songpair) {
		fs::path remove;
		try {
			std::string const& id = songpair.first;
			Song& song = songpair.second;
			std::cerr << "\n[" << id << "] " << song.artist << " - " << song.title << std::endl;
			fs::path path = safename(song.edition);
			path /= safename(song.artist + " - " + song.title);
			xmlpp::DomParser dom;
			dom.set_substitute_entities();
			{
				std::vector<char> tmp;
				Pak::files_t::const_iterator it = std::find_if(pak.files().begin(), pak.files().end(), Match("export/" + id + "/melody", ".xml"));
				if (it == pak.files().end()) throw std::runtime_error("Melody XML not found");
				it->second.get(tmp);
				std::string buf = xmlFix(tmp);
				bool succeeded = false;
				try {
					dom.parse_memory(buf);
					succeeded = true;
				} catch (...) {}
				if (!succeeded) {
					buf = Glib::convert(buf, "UTF-8", "ISO-8859-1"); // Convert to UTF-8
					dom.parse_memory(buf);
				}
			}
			if (song.tempo == 0.0) {
				xmlpp::NodeSet n = dom.get_document()->get_root_node()->find("/" + ns + "MELODY", nsmap);
				if (n.empty()) n = dom.get_document()->get_root_node()->find("/MELODY");
				if (n.empty()) throw std::runtime_error("Unable to find BPM info anywhere");
				xmlpp::Element& e = dynamic_cast<xmlpp::Element&>(*n[0]);
				std::string res = e.get_attribute("Resolution")->get_value();
				song.tempo = atof(e.get_attribute("Tempo")->get_value().c_str());
				if (res == "Semiquaver") void();
				else if (res == "Demisemiquaver") song.tempo *= 2.0;
				else throw std::runtime_error("Unknown tempo resolution: " + res);
			}
			fs::create_directories(path);
			remove = path;
			dom.get_document()->write_to_file((path / "notes.xml").string(), "UTF-8");
			Pak dataPak(song.dataPakName);
			std::cerr << ">>> Extracting and decoding music" << std::endl;
			music(song, dataPak[id + "/music.mib"], pak["export/" + id + "/music.mih"], path);
			std::cerr << ">>> Extracting lyrics" << std::endl;
			xmlpp::NodeSet sentences = dom.get_document()->get_root_node()->find("/" + ns + "MELODY/" + ns + "SENTENCE", nsmap);
			if(!sentences.empty()) {
				// Sentences not inside tracks (normal songs)
				std::cerr << "  >>> Solo track" << std::endl;
				saveTxtFile(sentences, path, song);
			} else {
				xmlpp::NodeSet tracks = dom.get_document()->get_root_node()->find("/" + ns + "MELODY/" + ns + "TRACK", nsmap);
				if( !tracks.empty()) {
					for( xmlpp::NodeSet::iterator it = tracks.begin() ; it != tracks.end() ; ++it ) {
						xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**it);
						std::string singer = elem.get_attribute("Artist")->get_value();
						std::cerr << "  >>> Track from " << singer << std::endl;
						sentences = dom.get_document()->get_root_node()->find((*it)->get_path() + "/" + ns + "SENTENCE", nsmap);
						saveTxtFile(sentences, path, song, singer);
					}
				} else {
					throw std::runtime_error("Unable to find any sentences in melody XML");
				}
			}
			std::cerr << ">>> Extracting cover image" << std::endl;
			try {
				SingstarCover c = SingstarCover(dvdPath + "/pack_ee.pak", boost::lexical_cast<unsigned int>(id));
				c.write(path.string() + "/cover.jpg");
			} catch (...) {}
			remove = "";
			if (video) {
				std::cerr << ">>> Extracting video" << std::endl;
				std::vector<char> ipudata;
				dataPak[id + "/movie.ipu"].get(ipudata);
				std::cerr << ">>> Converting video" << std::endl;
				IPUConv(ipudata, (path / "video.mpg").string());
				// FIXME: use some library (preferrably ffmpeg):
				if (oggcompress) {
					std::cerr << ">>> Compressing audio into music.ogg/vocals.ogg" << std::endl;
					std::string cmd = "oggenc \"" + (path / "music.wav").string() + "\"";
					std::cerr << cmd << std::endl;
					if (std::system(cmd.c_str()) == 0) { // FIXME: std::system return value is not portable
						fs::remove(path / "music.wav");
					}
					cmd = "oggenc \"" + (path / "vocals.wav").string() + "\"";
					std::cerr << cmd << std::endl;
					if (std::system(cmd.c_str()) == 0) { // FIXME: std::system return value is not portable
						fs::remove(path / "vocals.wav");
					}
				}
				if (mkvcompress) {
					std::cerr << ">>> Compressing video and audio into music.mkv" << std::endl;
					std::string cmd = "ffmpeg -i \"" + (path / "video.mpg").string() + "\" -vcodec libx264 -vpre hq -crf 25 -threads 0 -metadata album=\"" + song.edition + "\" -metadata author=\"" + song.artist + "\" -metadata comment=\"" + song.genre + "\" -metadata title=\"" + song.title + "\" \"" + (path / "video.m4v\"").string();
					std::cerr << cmd << std::endl;
					if (std::system(cmd.c_str()) == 0) { // FIXME: std::system return value is not portable
						fs::remove(path / "video.mpg");
					}
				}
			}
		} catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
			if (!remove.empty()) {
				std::cerr << "!!! Removing " << remove.string() << std::endl;
				fs::remove_all(remove);
			}
		}
	}
};

void get_node(const xmlpp::Node* node, std::string& genre, std::string& year, double& tempo)
{
	const xmlpp::ContentNode* nodeContent = dynamic_cast<const xmlpp::ContentNode*>(node);
	const xmlpp::TextNode* nodeText = dynamic_cast<const xmlpp::TextNode*>(node);
	const xmlpp::CommentNode* nodeComment = dynamic_cast<const xmlpp::CommentNode*>(node);

	if(nodeText && nodeText->is_white_space()) //Let's ignore the indenting - you don't always want to do this.
		return;
    
	//Treat the various node types differently: 
	if(nodeText || nodeComment || nodeContent)
	{
		// if any of these exist do nothing! :D      
	}
	else if(const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(node))
	{
		//A normal Element node:

		//Print attributes:
		const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
		for(xmlpp::Element::AttributeList::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter)
		{
			const xmlpp::Attribute* attribute = *iter;
			if (attribute->get_name() == "GENRE") genre = normalize(attribute->get_value());
			else if (attribute->get_name() == "YEAR") year = normalize(attribute->get_value());
		}
	}
  
	if(!nodeContent)
	{
		//Recurse through child nodes:
		xmlpp::Node::NodeList list = node->get_children();
		for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
		{
			get_node(*iter, genre, year, tempo); //recursive
		}
	}
}

struct FindSongs {
	std::string edition;
	std::map<std::string, Song> songs;
	FindSongs(std::string const& search = ""): m_search(search) {
	}
	void operator()(Pak::files_t::value_type const& p) {
		std::string name = p.first;
		if (name.substr(0, 17) == "export/config.xml"){
			// get the singstar edition
			xmlpp::DomParser dom;
			dom.set_substitute_entities();
			{
				std::vector<char> tmp;
				p.second.get(tmp);
				std::string buf = xmlFix(tmp);
				disableXMLLogger();
				try {
					dom.parse_memory(buf);
				} catch (...) {
					enableXMLLogger();
					buf = Glib::convert(buf, "UTF-8", "ISO-8859-1"); // Convert to UTF-8
					dom.parse_memory(buf);
				}
				enableXMLLogger();
			}
			ns.clear();

			// only get product name node
			xmlpp::NodeSet n = dom.get_document()->get_root_node()->find("/CONFIG/PRODUCT_DESC");
			if (n.empty()) { ns = "ss:"; n = dom.get_document()->get_root_node()->find("/ss:CONFIG/ss:PRODUCT_DESC", nsmap); }
			if (n.empty()) n = dom.get_document()->get_root_node()->find("/CONFIG/PRODUCT_NAME");
			if (n.empty()) { ns = "ss:"; n = dom.get_document()->get_root_node()->find("/ss:CONFIG/ss:PRODUCT_NAME", nsmap); }

			edition = n.empty() ? "Other" : dynamic_cast<xmlpp::Element&>(*n[0]).get_child_text()->get_content();
		}
		if (name.substr(0, 12) != "export/songs" || name.substr(name.size() - 4) != ".xml") return;
		xmlpp::DomParser dom;
		dom.set_substitute_entities();
		{
			std::vector<char> tmp;
			p.second.get(tmp);
			std::string buf = xmlFix(tmp);
			disableXMLLogger();
			try {
				dom.parse_memory(buf);
			} catch (...) {
				enableXMLLogger();
				buf = Glib::convert(buf, "UTF-8", "ISO-8859-1"); // Convert to UTF-8
				dom.parse_memory(buf);
			}
			enableXMLLogger();
		}
		ns.clear();

		xmlpp::NodeSet n = dom.get_document()->get_root_node()->find("/SONG_SET/SONG");
		if (n.empty()) {
			ns = "ss:";
			n = dom.get_document()->get_root_node()->find("/ss:SONG_SET/ss:SONG", nsmap);
		}
		Song s;
		s.dataPakName = dvdPath + "/pak_iop" + name[name.size() - 5] + ".pak";
		for (xmlpp::NodeSet::const_iterator it = n.begin(), end = n.end(); it != end; ++it) {
			xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**it);
			s.title = elem.get_attribute("TITLE")->get_value();
			s.artist = elem.get_attribute("PERFORMANCE_NAME")->get_value();
			if (!m_search.empty() && m_search != elem.get_attribute("ID")->get_value() && (s.artist + " - " + s.title).find(m_search) == std::string::npos) continue;
			xmlpp::Node* node = dynamic_cast<const xmlpp::Node*>((*it));
			get_node(node, s.genre, s.year, s.tempo); // get the values for genre, year and tempo

			xmlpp::NodeSet t = elem.find("TEMPO/@BPM");
			if (t.empty()) s.tempo = 0.0; // this will always be 0.0
			else s.tempo = atof(dynamic_cast<xmlpp::Attribute&>(*t[0]).get_value().c_str());

			s.edition = prettyEdition(edition);

			songs[elem.get_attribute("ID")->get_value()] = s;
		}
	}
  private:
	std::string m_search;
};

int main( int argc, char **argv) {
	if (argc < 2 || argc > 3) {
		std::cerr << "Usage: " << argv[0] << " dvdPath [track id or name]" << std::endl;
		return EXIT_FAILURE;
	}
	dvdPath = argv[1];
	std::string pack_ee = dvdPath + "/pack_ee.pak"; // Note: lower case (ISO-9660)
	if (!fs::exists(pack_ee)) {
		if (fs::exists(dvdPath + "/Pack_EE.PAK")) { // Note: capitalization (UDF)
			std::cerr <<
			  "Singstar DVDs have UDF and ISO-9660 filesystems on them. Your disc is mounted\n"
			  "as UDF and this causes some garbled data files, making ripping it impossible.\n\n"
			  "Please remount the disc as ISO-9660 and try again. E.g. on Linux:\n"
			  "# mount -t iso9660 /dev/cdrom " << dvdPath << std::endl;
		} else std::cerr << "No Singstar DVD found. Enter a path to a folder with pack_ee.pak in it." << std::endl;
		return EXIT_FAILURE;
	}
	nsmap["ss"] = "http://www.singstargame.com";
	Pak p(pack_ee);
	FindSongs f = std::for_each(p.files().begin(), p.files().end(), FindSongs(argc > 2 ? argv[2] : ""));
	std::cerr << f.songs.size() << " songs found" << std::endl;
	std::for_each(f.songs.begin(), f.songs.end(), Process(p));
}

