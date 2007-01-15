#include <songs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <algorithm>

#include <SDL/SDL_image.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_rotozoom.h>    

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
		return false;
	if(ordering1 == NULL)
		return true;
	if(ordering2 == NULL)
		return false;
	int cmp = strcmp(ordering1,ordering2);
	if( cmp < 0 )
		return true;
	else
		return false;
}

void CSong::parseFile( void )
{
	char buff[256];
	sprintf(buff,"%s/%s",path,filename);
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
				sprintf(syllable,"%s",buff+shift+2);
				tmp->syllable = syllable;
				if( tmp->note <= noteMin )
					noteMin = tmp->note;
				if( tmp->note >= noteMax )
					noteMax = tmp->note;
				notes.push_back(tmp);
				maxScore += tmp->length * tmp->type;
                                break;
			}
			case '-' : {
				TNote * tmp = new TNote();
				int timestamp;
				tmp->type = TYPE_NOTE_SLEEP;
				sscanf(buff+1,"%d",&timestamp);
				tmp->timestamp = relativeShift + timestamp;
				if(relative)
					relativeShift += timestamp;
				notes.push_back(tmp);
				break;
			}
		}
	}
	fclose(fp);
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
	sprintf(buff,"%s/%s",tmp->path,tmp->filename);
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
		} else if(!strncmp("#EDITION:",buff,9)) {
			int len = strlen(buff);
			char * edition = new char[len - 9];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(edition,buff+9,len - 9);
			tmp->edition = edition;
		} else if(!strncmp("#ARTIST:",buff,8)) {
			int len = strlen(buff);
			char * artist = new char[len - 8];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(artist,buff+8,len - 8);
			tmp->artist = artist;
		} else if(!strncmp("#MP3:",buff,5)) {
			int len = strlen(buff);
			char * mp3 = new char[len - 5];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(mp3,buff+5,len - 5);
			tmp->mp3 = mp3;
		} else if(!strncmp("#CREATOR:",buff,9)) {
			int len = strlen(buff);
			char * creator = new char[len - 9];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(creator,buff+9,len - 9);
			tmp->creator = creator;
		} else if(!strncmp("#GAP:",buff,5)) {
			sscanf(buff+5,"%f",&tmp->gap);
		} else if(!strncmp("#BPM:",buff,5)) {
			TBpm bpm;
			bpm.start = 0.0;
			// We replace ',' by '.' for internationalization
			char * comma = strchr(buff,',');
			if( comma != NULL )
				*comma = '.';
			sscanf(buff+5,"%f",&bpm.bpm);
			tmp->bpm.push_back(bpm);
		} else if(!strncmp("#VIDEO:",buff,7)) {
			int len = strlen(buff);
			char * video = new char[len - 7];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(video,buff+7,len - 7);
			tmp->video = video;
                } else if(!strncmp("#BACKGROUND:",buff,12)) {
                        int len = strlen(buff);
                        char * background = new char[len - 12];

                        if (buff[len-2] == '\r') len--;
                        buff[len-1]='\0'; // Replace the \n or \r with a \0
                        memcpy(background,buff+12,len - 12);
                        tmp->background = background;
                } else if(!strncmp("#VIDEOGAP:",buff,10)) {
			sscanf(buff+10,"%f",&tmp->videoGap);
		} else if(!strncmp("#RELATIVE:",buff,10)) {
			if( buff[10] == 'y' )
				tmp->relative = true;
			else
				tmp->relative = false;
		} else if(!strncmp("#COVER:",buff,7)) {
 			int len = strlen(buff);
			char * cover = new char[len - 7];

			if (buff[len-2] == '\r') len--;
			buff[len-1]='\0'; // Replace the \n or \r with a \0
			memcpy(cover,buff+7,len - 7);
			tmp->cover = cover;
                }
	}
	fclose(fp);
	return true;
}

CSongs::CSongs()
{
	DIR * dir;
	struct dirent* dirEntry;
	struct stat    info;
	char buff[1024];
	dir = opendir("songs/");
	order = 2;
	SDL_RWops *rwop_nocover = SDL_RWFromFile("images/no_cover.png", "rb");
	surface_nocover = IMG_LoadPNG_RW(rwop_nocover);
	SDL_FreeRW(rwop_nocover);

	while( (dirEntry = readdir(dir)) != NULL ) {
		if( dirEntry->d_name[0] == '.' )
			continue;
		if( !strcmp(dirEntry->d_name,"CVS") )
			continue;
		
		char * path = new char[1024];
		sprintf(path,"%s%s","songs/",dirEntry->d_name);
		stat(path,&info);
		if ( !S_ISDIR(info.st_mode) ) {
			delete[] path;
			continue;
		}

		fprintf(stdout,"Song directory \"%s\" detected\n",dirEntry->d_name);

		CSong * tmp = new CSong();

		// Set default orderType to title
		tmp->orderType = 2;
		tmp->path = path;
		char * txt = new char[strlen(dirEntry->d_name)+4+1];
		sprintf(txt,"%s.txt",dirEntry->d_name);
		tmp->filename = txt;
		if( !parseFile(tmp) ) {
			delete[] path;
			delete[] txt;
			delete tmp;
		} else {
			if(!tmp->cover) {
                            char * cover = new char[strlen(dirEntry->d_name)+4+1];
		            sprintf(cover,"%s.png",dirEntry->d_name);
		            tmp->cover = cover;
                        }

		        sprintf(buff,"%s/%s/%s","songs",dirEntry->d_name,tmp->cover);
		        SDL_RWops *rwop = SDL_RWFromFile(buff, "rb");
		        SDL_Surface * coverSurface = NULL;
                        if(strstr(tmp->cover, ".png"))
                            coverSurface = IMG_LoadPNG_RW(rwop);
		        else if(strstr(tmp->cover, ".jpg"))
                            coverSurface = IMG_LoadJPG_RW(rwop);
	                           
                        if( coverSurface == NULL )
			    tmp->coverSurf = surface_nocover;
		        else {
			    tmp->coverSurf = zoomSurface(coverSurface,(double) 256/coverSurface->w,(double) 256/coverSurface->h,1);
			    SDL_FreeRW(rwop);
		        }
		        
                        sprintf(buff,"%s/%s/%s","songs",dirEntry->d_name,tmp->background);
                        rwop = SDL_RWFromFile(buff, "rb");
                        
                        if (tmp->background) {
                            SDL_Surface * backgroundSurface = NULL;
                            if(strstr(tmp->background, ".png"))
                                backgroundSurface = IMG_LoadPNG_RW(rwop);
                            else if(strstr(tmp->background, ".jpg"))
                                backgroundSurface = IMG_LoadJPG_RW(rwop);
                                                 
                            if( backgroundSurface == NULL )
                                tmp->backgroundSurf = NULL;
                            else {
                                tmp->backgroundSurf = zoomSurface(backgroundSurface,(double)800/backgroundSurface->w,(double)600/backgroundSurface->h,1);
                                SDL_FreeRW(rwop);
                            }
                        }
                        
                        tmp->parseFile();
			songs.push_back(tmp);
		}
	}
	closedir(dir);
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
		for(unsigned int j = 0; j < songs[i]->notes.size(); j++) {
			delete[] songs[i]->notes[j]->syllable;
			delete songs[i]->notes[j];
		}
		if( surface_nocover != songs[i]->coverSurf )
			SDL_FreeSurface(songs[i]->coverSurf);
		if( songs[i]->backgroundSurf )
                        SDL_FreeSurface(songs[i]->backgroundSurf);
                delete songs[i];
	}
	delete surface_nocover;

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
