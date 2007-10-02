#include <boost/progress.hpp>
#include <glob.h>
#include <songs.h>
#include <screen.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

bool compareSongs(CSong const* left , CSong const* right) {
	if(left->orderType != right->orderType) throw std::logic_error("compareSongs: order mismatch");
	std::string ordering1;
	std::string ordering2;
	switch(left->orderType) {
	  case 0: //edition
		ordering1 = left->edition;
		ordering2 = right->edition;
		break;
	  case 1: //genre
		ordering1 = left->genre;
		ordering2 = right->genre;
		break;
	  case 2: //title
		ordering1 = left->title;
		ordering2 = right->title;
		break;
	  case 3: //artist
		ordering1 = left->artist;
		ordering2 = right->artist;
		break;
	  default:
		ordering1 = left->title;
		ordering2 = right->title;
		break;
	}
	if (ordering1 == ordering2) return left->index < right->index;
	return ordering1 < ordering2;
}

void CSong::parseFile() {
	int relativeShift = 0;
	maxScore = 0;
	std::string file = std::string(path) + "/" + filename;
	FILE * fp = fopen(file.c_str(), "r");
	char buff[256];
	while(fgets(buff,256,fp)) {
		switch(buff[0]) {
		  case '#': continue;
		  case 'E': break;
		  case 'F':
		  case ':':
		  case '*':
			{
				TNote * tmp = new TNote();
				int shift;
				int len = strlen(buff);
				char * syllable = new char[16];

				if (buff[0] == 'F') tmp->type = TYPE_NOTE_FREESTYLE;
				if (buff[0] == '*') tmp->type = TYPE_NOTE_GOLDEN;
				if (buff[0] == ':') tmp->type = TYPE_NOTE_NORMAL;

				if (buff[len-2] == '\r') len--;
				buff[len-1] = '\0'; // Replace the \n or \r with a \0
				sscanf(buff+1,"%d %d %d%n",&tmp->timestamp, &tmp->length , &tmp->note , &shift);
				tmp->timestamp += relativeShift;
				snprintf(syllable,16,"%s",buff+shift+2);
				tmp->syllable = syllable;
				// Avoid ":1 0 0" to mess noteMin
				if(tmp->length == 0) {
					delete[] tmp->syllable;
					delete tmp;
					break;
				}
				noteMin = std::min(noteMin, tmp->note);
				noteMax = std::max(noteMax, tmp->note);
				maxScore += tmp->length * tmp->type;
				tmp->curMaxScore = maxScore;
				notes.push_back(tmp);
				break;
			}
		  case '-':
			{
				TNote * tmp = new TNote();
				int timestamp;
				int sleep_end;
				tmp->type = TYPE_NOTE_SLEEP;
				int nbInt = sscanf(buff+1,"%d %d",&timestamp, &sleep_end);
				tmp->timestamp = relativeShift + timestamp;
				if(nbInt == 1) {
					tmp->length = 0;
				} else {
					tmp->length = sleep_end - timestamp;
				}
				if(relative) {
					if(nbInt == 1) {
						relativeShift += timestamp;
					} else {
						relativeShift += sleep_end;
					}
				}
				tmp->curMaxScore = maxScore;
				notes.push_back(tmp);
				break;
			}
		}
	}
	fclose(fp);
	// Adjust negative notes
	if(noteMin <= 0) {
		unsigned int shift = (((noteMin*-1)%12)+1)*12;
		noteMin += shift;
		noteMax += shift;
		for(unsigned i = 0; i < notes.size(); ++i) notes[i]->note+=shift;
	}
}

CSong::CSong():
	path(),
	filename(),
	genre(),
	edition(),
	title(),
	artist(),
	text(),
	creator(),
	cover(),
	coverSurf(),
	mp3(),
	background(),
	backgroundSurf(),
	video(),
	videoGap(),
	relative(),
	gap(),
	noteMin(256),
	noteMax(-256)
{}

bool CSongs::parseFile(CSong * tmp)
{
	int score = 0;
	std::string file = std::string(tmp->path) + "/" + tmp->filename;
	FILE * fp = fopen(file.c_str(), "r");
	if (!fp) {
		std::cout << "Cannot open " << file << std::endl;
		return false;
	}
	char buff[256];
	while (fgets(buff,256,fp)) {
		if (buff[0] != '#') continue;
		if (!strncmp("#TITLE:",buff,7)) {
			int len = strlen(buff);
			char * title = new char[len - 7];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(title,buff+7,len - 7);
			tmp->title = title;
			score++;
		} else if (!strncmp("#EDITION:",buff,9)) {
			int len = strlen(buff);
			char * edition = new char[len - 9];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(edition,buff+9,len - 9);
			tmp->edition = edition;
			score++;
		} else if (!strncmp("#ARTIST:",buff,8)) {
			int len = strlen(buff);
			char * artist = new char[len - 8];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(artist,buff+8,len - 8);
			tmp->artist = artist;
			score++;
		} else if (!strncmp("#MP3:",buff,5)) {
			int len = strlen(buff);
			char * mp3 = new char[len - 5];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(mp3,buff+5,len - 5);
			tmp->mp3 = mp3;
			score++;
		} else if (!strncmp("#CREATOR:",buff,9)) {
			int len = strlen(buff);
			char * creator = new char[len - 9];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(creator,buff+9,len - 9);
			tmp->creator = creator;
			score++;
		} else if (!strncmp("#GAP:",buff,5)) {
			sscanf(buff+5,"%f",&tmp->gap);
			score++;
		} else if (!strncmp("#BPM:",buff,5)) {
			TBpm bpm;
			bpm.start = 0.0;
			// We replace ',' by '.' for internationalization
			char * comma = strchr(buff,',');
			if(comma) *comma = '.';
			sscanf(buff+5,"%f",&bpm.bpm);
			tmp->bpm.push_back(bpm);
			score++;
		} else if (!strncmp("#VIDEO:",buff,7)) {
			int len = strlen(buff);
			char * video = new char[len - 7];
			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(video,buff+7,len - 7);
			tmp->video = video;
			score++;
			} else if(!strncmp("#BACKGROUND:",buff,12)) {
				int len = strlen(buff);
				char * background = new char[len - 12];
				if (buff[len-2] == '\r') len--;
				buff[len-1]='\0'; // Replace the \n or \r with a \0
				memcpy(background,buff+12,len - 12);
				tmp->background = background;
				score++;
			} else if(!strncmp("#VIDEOGAP:",buff,10)) {
				sscanf(buff+10,"%f",&tmp->videoGap);
				score++;
			} else if(!strncmp("#RELATIVE:",buff,10)) {
				tmp->relative = (buff[10] == 'y' || buff[10] == 'Y');
				score++;
		} else if(!strncmp("#COVER:",buff,7)) {
 			int len = strlen(buff);
			char * cover = new char[len - 7];
			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(cover,buff+7,len - 7);
			tmp->cover = cover;
			score++;
		}
	}
	fclose(fp);
	return score != 0;
}

void CSong::loadCover() {
	if (coverSurf || cover.empty()) return;
	double width = CScreenManager::getSingletonPtr()->getWidth();
	double height = CScreenManager::getSingletonPtr()->getHeight();
	std::string file = std::string(path) + "/" + cover;
	std::string::size_type extpos = file.rfind('.');
	std::string ext = (extpos == std::string::npos) ? "" : file.substr(extpos);
	SDL_RWops* rwop = SDL_RWFromFile(file.c_str(), "rb");
	SDL_Surface* surf = NULL;
	if (ext == ".jpg" || ext == ".JPG" || ext == ".jpeg") surf = IMG_LoadJPG_RW(rwop);
	else if (ext == ".png" || ext == ".PNG") surf = IMG_LoadPNG_RW(rwop);
	//else std::cout << "Cover image file " << file << " has unknown extension" << std::endl;
	if (rwop) SDL_RWclose(rwop);
	if (surf == NULL) coverSurf = NULL;
	else {
		// Here we want to have cover of 256x256 in 800x600 and scale it if the resolution is different
		double w = width * 256 / 800;
		double h = height * 256 / 600;
		coverSurf = zoomSurface(surf, w / surf->w, h / surf->h, 1);
		SDL_FreeSurface(surf);
	}
}

void CSong::loadBackground() {
	if (backgroundSurf || background.empty()) return;
	double width = CScreenManager::getSingletonPtr()->getWidth();
	double height = CScreenManager::getSingletonPtr()->getHeight();
	std::string file = std::string(path) + "/" + background;
	std::string::size_type extpos = file.rfind('.');
	std::string ext = (extpos == std::string::npos) ? "" : file.substr(extpos);
	SDL_RWops* rwop = SDL_RWFromFile(file.c_str(), "rb");
	SDL_Surface* surf = NULL;
	if (ext == ".jpg" || ext == ".JPG" || ext == ".jpeg") surf = IMG_LoadJPG_RW(rwop);
	else if (ext == ".png" || ext == ".PNG") surf = IMG_LoadPNG_RW(rwop);
	//else std::cout << "Background image file " << file << " has unknown extension" << std::endl;
	if (rwop) SDL_RWclose(rwop);
	if (surf == NULL) backgroundSurf = NULL;
	else {
		backgroundSurf = zoomSurface(surf, width / surf->w, height / surf->h, 1);
		SDL_FreeSurface(surf);
	}
}

void CSong::unloadCover() {
	if (coverSurf) SDL_FreeSurface(coverSurf);
	coverSurf = NULL;
}

void CSong::unloadBackground() {
	if (backgroundSurf) SDL_FreeSurface(backgroundSurf);
	backgroundSurf = NULL;
}

CSongs::CSongs(std::set<std::string> const& songdirs) {
	glob_t _glob;
	order = 2;

	SDL_RWops* rwop_nocover = SDL_RWFromFile(CScreenManager::getSingletonPtr()->getThemePathFile("no_cover.png").c_str(), "rb");
	SDL_Surface* surface_nocover_tmp = IMG_LoadPNG_RW(rwop_nocover);
	int w = CScreenManager::getSingletonPtr()->getWidth()*256/800;
	int h = CScreenManager::getSingletonPtr()->getHeight()*256/600;
	surface_nocover = zoomSurface(surface_nocover_tmp,(double)w/surface_nocover_tmp->w,(double)h/surface_nocover_tmp->h,1);
	SDL_FreeSurface(surface_nocover_tmp);
	if (rwop_nocover) SDL_RWclose(rwop_nocover);
	if (surface_nocover == NULL) {
		printf("IMG_LoadPNG_RW: %s\n", IMG_GetError());
		return;
	}
	for (std::set<std::string>::const_iterator it = songdirs.begin(); it != songdirs.end(); ++it) {
		std::string pattern = *it + "/*/*.[tT][xX][tT]";
		std::cout << "Scanning " << *it << " for songs" << std::endl;
		glob (pattern.c_str(), GLOB_NOSORT, NULL, &_glob);
		std::cout << "Found " << _glob.gl_pathc << " possible song file(s)..." << std::endl;
		for(unsigned int i = 0 ; i < _glob.gl_pathc ; i++) {
			char * path = new char[1024];
			char * txtfilename;
			txtfilename = strrchr(_glob.gl_pathv[i],'/'); txtfilename[0] = '\0'; txtfilename++;
			sprintf(path,"%s",_glob.gl_pathv[i]);
			std::cout << "Loading song: \"" << strrchr(_glob.gl_pathv[i],'/') + 1 << "\"... ";
			CSong * tmp = new CSong();
			// Set default orderType to title
			tmp->orderType = 2;
			tmp->path = path;
			char * txt = new char[strlen(txtfilename)+1];
			sprintf(txt,"%s",txtfilename); // safe sprintf
			tmp->filename = txt;
			if(!parseFile(tmp)) {
				std::cout << "FAILED" << std::endl;
				delete[] path;
				delete[] txt;
				delete tmp;
			} else {
				std::cout << "OK" << std::endl;
				tmp->parseFile();
				tmp->index = songs.size();
				songs.push_back(tmp);
			}
		}
		globfree(&_glob);
	}
	boost::progress_display progress(songs.size());
	for(unsigned int i = 0; i < songs.size(); i++) {
		CSong& song = *getSong(i);
		song.loadCover();
		++progress;
		// Is now loaded only when needed: song.loadBackground();
	}
}

CSongs::~CSongs() {
	for(unsigned int i = 0; i < songs.size(); i++) {
		for(unsigned int j = 0; j < songs[i]->notes.size(); j++) {
			delete[] songs[i]->notes[j]->syllable;
			delete songs[i]->notes[j];
		}
		delete songs[i];
	}
	SDL_FreeSurface(surface_nocover);
}

CSong* CSongs::getSong(unsigned int i) {
	return songs.at(i);
}

void CSongs::sortByEdition()
{
	order = 0;
	for(unsigned int i = 0; i < songs.size(); i++)
		songs[i]->orderType = 0;
	sort(songs.begin(), songs.end(),compareSongs);
}
void CSongs::sortByGenre()
{
	order = 1;
	for(unsigned int i = 0; i < songs.size(); i++)
		songs[i]->orderType = 1;
	sort(songs.begin(), songs.end(),compareSongs);
}
void CSongs::sortByTitle()
{
	order = 2;
	for(unsigned int i = 0; i < songs.size(); i++)
		songs[i]->orderType = 2;
	sort(songs.begin(), songs.end(),compareSongs);
}
void CSongs::sortByArtist()
{
	order = 3;
	for(unsigned int i = 0; i < songs.size(); i++)
		songs[i]->orderType = 3;
	sort(songs.begin(), songs.end(),compareSongs);
}
