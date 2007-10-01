#include <glob.h>
#include <songs.h>
#include <screen.h>
#include <iostream>

bool compareSongs( CSong * left , CSong * right)
{
	if( left->orderType != right->orderType )
		fprintf(stderr,"Order mismatch\n");
	char * ordering1;
	char * ordering2;
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
	// VERY IMPORTANT, if equal compareSongs MUST return false
	if(ordering1 == NULL && ordering2 == NULL)
		return (left->index < right->index);
	if(ordering1 == NULL)
		return true;
	if(ordering2 == NULL)
		return false;
	int cmp = strcmp(ordering1,ordering2);
	if( cmp < 0 )
		return true;
	else if( cmp > 0 )
		return false;
	else
		return (left->index < right->index);
}

void CSong::parseFile( void )
{
	char buff[256];
	snprintf(buff,256,"%s/%s",path,filename);
	int relativeShift = 0;
        maxScore = 0;
	FILE * fp = fopen(buff,"r");
	while(fgets(buff,256,fp)) {
		switch( buff[0] ) {
			case '#' :
				continue;
			case 'E' :
				break;
			case 'F' :
                        case ':' :
			case '*' : {
				TNote * tmp = new TNote();
				int shift;
				int len = strlen(buff);
				char * syllable = new char[16];

                                if (buff[0] == 'F')
                                    tmp->type = TYPE_NOTE_FREESTYLE;
                                if (buff[0] == '*')
                                    tmp->type = TYPE_NOTE_GOLDEN;
                                if (buff[0] == ':')
                                    tmp->type = TYPE_NOTE_NORMAL;

                                if (buff[len-2] == '\r') len--;
				buff[len-1] = '\0'; // Replace the \n or \r with a \0
				sscanf(buff+1,"%d %d %d%n",&tmp->timestamp, &tmp->length , &tmp->note , &shift);
				tmp->timestamp += relativeShift;
				snprintf(syllable,16,"%s",buff+shift+2);
				tmp->syllable = syllable;
				// Avoid ":1 0 0" to mess noteMin
				if( tmp->length == 0 ) {
					delete[] tmp->syllable;
					delete tmp;
					break;
				}
				if( tmp->note <= noteMin )
					noteMin = tmp->note;
				if( tmp->note >= noteMax )
					noteMax = tmp->note;
				maxScore += tmp->length * tmp->type;
				tmp->curMaxScore = maxScore;
				notes.push_back(tmp);
                                break;
			}
			case '-' : {
				TNote * tmp = new TNote();
				int timestamp;
				int sleep_end;
				tmp->type = TYPE_NOTE_SLEEP;
				int nbInt = sscanf(buff+1,"%d %d",&timestamp, &sleep_end);
				tmp->timestamp = relativeShift + timestamp;
				if( nbInt == 1 ) {
					tmp->length = 0;
				} else {
					tmp->length = sleep_end - timestamp;
				}
				if(relative) {
					if( nbInt == 1 ) {
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
	// Adjust negativ notes
	if( noteMin <= 0 ) {
		unsigned int shift = (((noteMin*-1)%12)+1)*12;
		noteMin+= shift;
		noteMax+= shift;

		for( unsigned int i = 0 ; i < notes.size() ; i++ )
			notes[i]->note+=shift;
	}
}

CSong::CSong()
{
	path = NULL;
	filename = NULL;
	genre = NULL;
	edition = NULL;
	title = NULL;
	artist = NULL;
	text = NULL;
	creator = NULL;
	cover = NULL;
	mp3 = NULL;
	background = NULL;
	backgroundSurf = NULL;
	coverSurf = NULL;
        video = NULL;
	noteMin = 256;
	noteMax = -256;
	relative = false;
	videoGap=0;
	gap=0;
}

bool CSongs::parseFile( CSong * tmp )
{
	char buff[256];
	int score = 0;
	snprintf(buff,256,"%s/%s",tmp->path,tmp->filename);
	FILE * fp = fopen(buff,"r");
	if(!fp) {
		fprintf(stderr , "Cannot open \"%s\"\n",buff);
		return false;
	}
	while(fgets(buff,256,fp)) {
		if(buff[0] != '#' )
			continue;
		if(!strncmp("#TITLE:",buff,7)) {
			int len = strlen(buff);
			char * title = new char[len - 7];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(title,buff+7,len - 7);
			tmp->title = title;
			score++;
		} else if(!strncmp("#EDITION:",buff,9)) {
			int len = strlen(buff);
			char * edition = new char[len - 9];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(edition,buff+9,len - 9);
			tmp->edition = edition;
			score++;
		} else if(!strncmp("#ARTIST:",buff,8)) {
			int len = strlen(buff);
			char * artist = new char[len - 8];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(artist,buff+8,len - 8);
			tmp->artist = artist;
			score++;
		} else if(!strncmp("#MP3:",buff,5)) {
			int len = strlen(buff);
			char * mp3 = new char[len - 5];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(mp3,buff+5,len - 5);
			tmp->mp3 = mp3;
			score++;
		} else if(!strncmp("#CREATOR:",buff,9)) {
			int len = strlen(buff);
			char * creator = new char[len - 9];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(creator,buff+9,len - 9);
			tmp->creator = creator;
			score++;
		} else if(!strncmp("#GAP:",buff,5)) {
			sscanf(buff+5,"%f",&tmp->gap);
			score++;
		} else if(!strncmp("#BPM:",buff,5)) {
			TBpm bpm;
			bpm.start = 0.0;
			// We replace ',' by '.' for internationalization
			char * comma = strchr(buff,',');
			if( comma != NULL )
				*comma = '.';
			sscanf(buff+5,"%f",&bpm.bpm);
			tmp->bpm.push_back(bpm);
			score++;
		} else if(!strncmp("#VIDEO:",buff,7)) {
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
			if( buff[10] == 'y'  || buff[10] == 'Y' )
				tmp->relative = true;
			else
				tmp->relative = false;
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
	if( score == 0 )
		return false;
	else
		return true;
}

void CSongs::loadCover( unsigned int i, unsigned int width, unsigned int height)
{
	if( songs[i]->coverSurf == NULL ) {
		CSong * song = songs[i];
		char buff[1024];
		snprintf(buff,1024,"%s/%s",song->path,song->cover);
		SDL_RWops *rwop = NULL;
		rwop = SDL_RWFromFile(buff, "rb");
		SDL_Surface* coverSurface = NULL;
		if(song->cover != NULL && strstr(song->cover, ".png"))
			coverSurface = IMG_LoadPNG_RW(rwop);
		else if(song->cover != NULL && strstr(song->cover, ".jpg"))
			coverSurface = IMG_LoadJPG_RW(rwop);
		if( rwop != NULL )
			SDL_RWclose(rwop);

		if( coverSurface == NULL )
			song->coverSurf = surface_nocover;
		else {
			// Here we want to have cover of 256x256 in 800x600 and scale it if the resolution is different
			int w = width*256/800;
			int h = height*256/600;
			song->coverSurf = zoomSurface(coverSurface,(double) w/coverSurface->w,(double) h/coverSurface->h,1);
			SDL_FreeSurface(coverSurface);
		}
	}
}

void CSongs::loadBackground( unsigned int i, unsigned int width, unsigned int height)
{
	if( songs[i]->backgroundSurf == NULL && songs[i]->background != NULL ) {
		CSong * song = songs[i];
		char buff[1024];
		snprintf(buff,1024,"%s/%s",song->path,song->background);
		SDL_RWops *rwop = NULL;
		rwop = SDL_RWFromFile(buff, "rb");
		SDL_Surface* backgroundSurface = NULL;
		if(strstr(song->background, ".png"))
			backgroundSurface = IMG_LoadPNG_RW(rwop);
		else if(strstr(song->background, ".jpg"))
			backgroundSurface = IMG_LoadJPG_RW(rwop);
		if( rwop != NULL )
			SDL_RWclose(rwop);

		if( backgroundSurface == NULL )
			song->backgroundSurf = NULL;
		else {
			song->backgroundSurf = zoomSurface(backgroundSurface,(double) width/backgroundSurface->w,(double) height/backgroundSurface->h,1);
			SDL_FreeSurface(backgroundSurface);
		}
	}
}

void CSongs::unloadCover( unsigned int i)
{
	if( songs[i]->coverSurf != NULL && songs[i]->coverSurf != surface_nocover ) {
		SDL_FreeSurface(songs[i]->coverSurf);
	}
	songs[i]->coverSurf = NULL;
}

void CSongs::unloadBackground( unsigned int i)
{
	if( songs[i]->backgroundSurf != NULL ) {
		SDL_FreeSurface(songs[i]->backgroundSurf);
	}
	songs[i]->backgroundSurf = NULL;
}

CSongs::CSongs(std::set<std::string> const& songdirs)
{
	glob_t _glob;
	order = 2;

	SDL_RWops *rwop_nocover = NULL;
	rwop_nocover = SDL_RWFromFile(CScreenManager::getSingletonPtr()->getThemePathFile("no_cover.png").c_str(), "rb");

	{
	SDL_Surface * surface_nocover_tmp = NULL;
	surface_nocover_tmp = IMG_LoadPNG_RW(rwop_nocover);
	int w = CScreenManager::getSingletonPtr()->getWidth()*256/800;
	int h = CScreenManager::getSingletonPtr()->getHeight()*256/600;
	surface_nocover = zoomSurface(surface_nocover_tmp,(double)w/surface_nocover_tmp->w,(double)h/surface_nocover_tmp->h,1);
	SDL_FreeSurface(surface_nocover_tmp);
	if( rwop_nocover != NULL )
		SDL_RWclose(rwop_nocover);
	}

	if( surface_nocover == NULL ) {
		printf("IMG_LoadPNG_RW: %s\n", IMG_GetError());
		return;
	}

	for (std::set<std::string>::const_iterator it = songdirs.begin(); it != songdirs.end(); ++it) {
		std::string pattern = *it + "/*/*.[tT][xX][tT]";
		std::cout << "Scanning " << *it << " for songs" << std::endl;
		glob ( pattern.c_str(), GLOB_NOSORT, NULL, &_glob);
		std::cout << "Found " << _glob.gl_pathc << " possible song file(s)..." << std::endl;
		for( unsigned int i = 0 ; i < _glob.gl_pathc ; i++ ) {
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
			if( !parseFile(tmp) ) {
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
	
	for(unsigned int i = 0; i < songs.size(); i++) {
		loadCover(i,CScreenManager::getSingletonPtr()->getWidth(),CScreenManager::getSingletonPtr()->getHeight());
		loadBackground(i,CScreenManager::getSingletonPtr()->getWidth(),CScreenManager::getSingletonPtr()->getHeight());
	}
}

CSongs::~CSongs()
{
	for(unsigned int i = 0; i < songs.size(); i++) {
		delete[] songs[i]->mp3;
		delete[] songs[i]->path;
		delete[] songs[i]->filename;
		delete[] songs[i]->cover;
		delete[] songs[i]->title;
		delete[] songs[i]->artist;
		delete[] songs[i]->edition;
		delete[] songs[i]->creator;
		delete[] songs[i]->video;
		delete[] songs[i]->background;
		for(unsigned int j = 0; j < songs[i]->notes.size(); j++) {
			delete[] songs[i]->notes[j]->syllable;
			delete songs[i]->notes[j];
		}
		unloadCover(i);
		unloadBackground(i);
                delete songs[i];
	}
	SDL_FreeSurface(surface_nocover);

}

CSong * CSongs::getSong( unsigned int i )
{
	if( i >= songs.size())
		return NULL;
	else
		return songs[i];
}

void CSongs::sortByEdition( void )
{
	order = 0;
	for(unsigned int i = 0; i < songs.size(); i++)
		songs[i]->orderType = 0;
	sort(songs.begin(), songs.end(),compareSongs);
}
void CSongs::sortByGenre( void )
{
	order = 1;
	for(unsigned int i = 0; i < songs.size(); i++)
		songs[i]->orderType = 1;
	sort(songs.begin(), songs.end(),compareSongs);
}
void CSongs::sortByTitle( void )
{
	order = 2;
	for(unsigned int i = 0; i < songs.size(); i++)
		songs[i]->orderType = 2;
	sort(songs.begin(), songs.end(),compareSongs);
}
void CSongs::sortByArtist( void )
{
	order = 3;
	for(unsigned int i = 0; i < songs.size(); i++)
		songs[i]->orderType = 3;
	sort(songs.begin(), songs.end(),compareSongs);
}
