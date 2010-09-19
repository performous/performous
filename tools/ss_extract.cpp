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

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include "chc_decode.hh"
#include "ss_cover.hh"

#include "ss_helpers.hh"

namespace fs = boost::filesystem;

struct Song {
	std::string dataPakName, title, artist, genre, edition, year;
	fs::path path, music, vocals, video, background, cover;
	unsigned samplerate;
	double tempo;
	bool pal;
	Song(): samplerate(), tempo() {}
};

#include "ss_binary.hh"

std::string dvdPath;
std::ofstream txtfile;
int ts = 0;
int sleepts = -1;
const bool video = true;
const bool mkvcompress = false;
const bool oggcompress = true;

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
	unsigned note = boost::lexical_cast<unsigned>(elem.get_attribute("MidiNote")->get_value().c_str());
	unsigned duration = boost::lexical_cast<unsigned>(elem.get_attribute("Duration")->get_value().c_str());
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
	// FIXME: Get rid of this or use SSDom's find
	xmlpp::Node::PrefixNsMap nsmap;
	nsmap["ss"] = "http://www.singstargame.com";
	xmlpp::NodeSet n = elem.find("ss:NOTE", nsmap);
	if (n.empty()) n = elem.find("NOTE");
	if (sleepts != -1) sleepts = ts;
	std::for_each(n.begin(), n.end(), parseNote);
}

struct Match {
	std::string left, right;
	Match(std::string l, std::string r): left(l), right(r) {}
	bool operator()(Pak::files_t::value_type const& f) {
		std::string n = f.first;
		return n.substr(0, left.size()) == left && n.substr(n.size() - right.size()) == right;
	}
};

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
	if (!song.video.empty()) txtfile << "#VIDEO:" << filename(song.video) << std::endl;
	if (!song.cover.empty()) txtfile << "#COVER:" << filename(song.cover) << std::endl;
	//txtfile << "#BACKGROUND:background.jpg" << std::endl;
	txtfile << "#BPM:" << song.tempo << std::endl;
	ts = 0;
	sleepts = -1;
	std::for_each(sentence.begin(), sentence.end(), parseSentence);
	txtfile << 'E' << std::endl;
	txtfile.close();
}

ChcDecode chc_decoder;

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
			SSDom dom;
			{
				std::vector<char> tmp;
				Pak::files_t::const_iterator it = std::find_if(pak.files().begin(), pak.files().end(), Match("export/" + id + "/melody", ".xml"));
				if (it == pak.files().end()) {
					it = std::find_if(pak.files().begin(), pak.files().end(), Match("export/melodies_10", ".chc"));
					if (it == pak.files().end()) throw std::runtime_error("Melody XML not found");
					it->second.get(tmp);
					dom.load(chc_decoder.getMelody(&tmp[0], tmp.size(), boost::lexical_cast<unsigned int>(id)));
				} else {
					it->second.get(tmp);
					dom.load(xmlFix(tmp));
				}
			}
			if (song.tempo == 0.0) {
				xmlpp::NodeSet n;
				dom.find("/ss:MELODY", n) || dom.find("/MELODY", n);
				if (n.empty()) throw std::runtime_error("Unable to find BPM info");
				xmlpp::Element& e = dynamic_cast<xmlpp::Element&>(*n[0]);
				std::string res = e.get_attribute("Resolution")->get_value();
				song.tempo = boost::lexical_cast<double>(e.get_attribute("Tempo")->get_value().c_str());
				if (res == "Semiquaver") {}
				else if (res == "Demisemiquaver") song.tempo *= 2.0;
				else throw std::runtime_error("Unknown tempo resolution: " + res);
			}
			fs::create_directories(path);
			remove = path;
			dom.get_document()->write_to_file((path / "notes.xml").string(), "UTF-8");
			Pak dataPak(song.dataPakName);
			std::cerr << ">>> Extracting and decoding music" << std::endl;
			try {
				music(song, dataPak[id + "/music.mib"], pak["export/" + id + "/music.mih"], path);
			} catch (...) {
				music_us(song, dataPak[id + "/mus+vid.iav"], dataPak[id + "/mus+vid.ind"], path);
			}
			std::cerr << ">>> Extracting cover image" << std::endl;
			try {
				SingstarCover c = SingstarCover(dvdPath + "/pack_ee.pak", boost::lexical_cast<unsigned int>(id));
				c.write(path.string() + "/cover.jpg");
				song.cover = path / "cover.jpg";
			} catch (...) {}
			remove = "";
			// FIXME: use some library (preferrably ffmpeg):
			if (oggcompress) {
				if( !song.music.empty() ) {
					std::cerr << ">>> Compressing audio into music.ogg" << std::endl;
					std::string cmd = "oggenc \"" + song.music.string() + "\"";
					std::cerr << cmd << std::endl;
					if (std::system(cmd.c_str()) == 0) { // FIXME: std::system return value is not portable
						fs::remove(song.music);
						song.music = path / ("music.ogg");
					}
				}
				if( !song.vocals.empty() ) {
					std::cerr << ">>> Compressing audio into vocals.ogg" << std::endl;
					std::string cmd = "oggenc \"" + song.vocals.string() + "\"";
					std::cerr << cmd << std::endl;
					if (std::system(cmd.c_str()) == 0) { // FIXME: std::system return value is not portable
						fs::remove(song.vocals);
						song.vocals = path / ("vocals.ogg");
					}
				}
			}
			if (video) {
				std::cerr << ">>> Extracting video" << std::endl;
				try {
					std::vector<char> ipudata;
					dataPak[id + "/movie.ipu"].get(ipudata);
					std::cerr << ">>> Converting video" << std::endl;
					IPUConv(ipudata, (path / "video.mpg").string());
					song.video = path / "video.mpg";
				} catch (...) {
					std::cerr << "  >>> European DVD failed, trying American (WIP)" << std::endl;
					try {
						video_us(song, dataPak[id + "/mus+vid.iav"], dataPak[id + "/mus+vid.ind"], path);
					} catch (std::exception& e) {
						std::cerr << "!!! Unable to extract video: " << e.what() << std::endl;
						song.video = "";
					}
				}
				if (mkvcompress) {
					std::cerr << ">>> Compressing video and audio into music.mkv" << std::endl;
					std::string cmd = "ffmpeg -i \"" + (path / "video.mpg").string() + "\" -vcodec libx264 -vpre hq -crf 25 -threads 0 -metadata album=\"" + song.edition + "\" -metadata author=\"" + song.artist + "\" -metadata comment=\"" + song.genre + "\" -metadata title=\"" + song.title + "\" \"" + (path / "video.m4v\"").string();
					std::cerr << cmd << std::endl;
					if (std::system(cmd.c_str()) == 0) { // FIXME: std::system return value is not portable
						fs::remove(path / "video.mpg");
						song.video = path / "video.m4v";
					}
				}
			}

			std::cerr << ">>> Extracting lyrics" << std::endl;
			xmlpp::NodeSet sentences;
			if(dom.find("/ss:MELODY/ss:SENTENCE", sentences)) {
				// Sentences not inside tracks (normal songs)
				std::cerr << "  >>> Solo track" << std::endl;
				saveTxtFile(sentences, path, song);
			} else {
				xmlpp::NodeSet tracks;
				if (!dom.find("/ss:MELODY/ss:TRACK", tracks)) throw std::runtime_error("Unable to find any sentences in melody XML");
				for (xmlpp::NodeSet::iterator it = tracks.begin(); it != tracks.end(); ++it ) {
					xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**it);
					std::string singer = elem.get_attribute("Artist")->get_value();
					std::cerr << "  >>> Track from " << singer << std::endl;
					dom.find(elem, "ss:SENTENCE", sentences);
					saveTxtFile(sentences, path, song, singer);
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

void get_node(const xmlpp::Node* node, std::string& genre, std::string& year)
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
			get_node(*iter, genre, year); //recursive
		}
	}
}

struct FindSongs {
	std::string edition;
	std::string language;
	std::map<std::string, Song> songs;
	FindSongs(std::string const& search = ""): m_search(search) {}
	void operator()(Pak::files_t::value_type const& p) {
		std::string name = p.first;
		if (name.substr(0, 17) == "export/config.xml"){
			SSDom dom(p.second);  // Read config XML
			// Load decryption keys required for some SingStar games (since 2009 or so)
			std::string keys[4];
			dom.getValue("/ss:CONFIG/ss:PRODUCT_NAME", keys[0]);
			dom.getValue("/ss:CONFIG/ss:PRODUCT_CODE", keys[1]);
			dom.getValue("/ss:CONFIG/ss:TERRITORY", keys[2]);
			dom.getValue("/ss:CONFIG/ss:DEFAULT_LANG", keys[3]);
			chc_decoder.load(keys);
			// Get the singstar edition, use PRODUCT_NAME as fallback for SS Original and SS Party
			if (!dom.getValue("/ss:CONFIG/ss:PRODUCT_DESC", edition)) edition = keys[0];
			if (edition.empty()) throw std::runtime_error("No PRODUCT_DESC or PRODUCT_NAME found");
			edition = prettyEdition(edition);
			std::cout << "### " << edition << std::endl;
			// Get language if available
			language = keys[3];
		}

		if (name.substr(0, 12) != "export/songs" || name.substr(name.size() - 4) != ".xml") return;
		SSDom dom(p.second);  // Read song XML


		xmlpp::NodeSet n;
		dom.find("/ss:SONG_SET/ss:SONG", n);
		Song s;
		s.dataPakName = dvdPath + "/pak_iop" + name[name.size() - 5] + ".pak";
		s.edition = edition;
		for (xmlpp::NodeSet::const_iterator it = n.begin(), end = n.end(); it != end; ++it) {
			// Extract song info
			xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**it);
			s.title = elem.get_attribute("TITLE")->get_value();
			s.artist = elem.get_attribute("PERFORMANCE_NAME")->get_value();
			if (!m_search.empty() && m_search != elem.get_attribute("ID")->get_value() && (s.artist + " - " + s.title).find(m_search) == std::string::npos) continue;
			xmlpp::Node const* node = dynamic_cast<xmlpp::Node const*>(*it);
			get_node(node, s.genre, s.year); // get the values for genre and year
			// Get video FPS
			double fps = 25.0;
			xmlpp::NodeSet fr;
			if (dom.find(elem, "ss:VIDEO/@FRAME_RATE", fr))
			  fps = boost::lexical_cast<double>(dynamic_cast<xmlpp::Attribute&>(*fr[0]).get_value().c_str());
			if (fps == 25.0) s.pal = true;
			// Store song info to songs container
			songs[elem.get_attribute("ID")->get_value()] = s;
		}
	}
  private:
	std::string m_search;
};

int main( int argc, char **argv) {
	std::string video, audio, song;
	namespace po = boost::program_options;
	po::options_description opt("Options");
	opt.add_options()
	  ("help,h", "you are viewing it")
	  ("dvd", po::value<std::string>(&dvdPath), "path to Singstar DVD root")
	  ("list,l", "list tracks only")
	  ("song", po::value<std::string>(&song), "only extract the given track (ID or partial name)")
	  ("video", po::value<std::string>(&video)->default_value("mkv"), "specify video format (none, mkv, mpeg2)")
	  ("audio", po::value<std::string>(&audio)->default_value("ogg"), "specify audio format (none, ogg, wav)")
	  ;
	// Process the first flagless option as dvd, the second as song
	po::positional_options_description pos;
	pos.add("dvd", 1);
	pos.add("song", 1);
	po::options_description cmdline;
	cmdline.add(opt);
	po::variables_map vm;
	// Load the arguments
	try {
		po::store(po::command_line_parser(argc, argv).options(opt).positional(pos).run(), vm);
		po::notify(vm);
		if (dvdPath.empty()) throw std::runtime_error("No Singstar DVD path specified. Enter a path to a folder with pack_ee.pak in it.");
		// TODO: process audio and video options and throw if they have incorrect values
	} catch (std::exception& e) {
		std::cout << cmdline << std::endl;
		std::cout << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
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
	Pak p(pack_ee);
	FindSongs f = std::for_each(p.files().begin(), p.files().end(), FindSongs(song));
	std::cerr << f.songs.size() << " songs found" << std::endl;
	if (vm.count("list")) {
		for( std::map<std::string, Song>::const_iterator it = f.songs.begin() ; it != f.songs.end();  ++it) {
			std::cout << "[" << it->first << "] " << it->second.artist << " - " << it->second.title << std::endl;
		}
	}
	else std::for_each(f.songs.begin(), f.songs.end(), Process(p));
}

