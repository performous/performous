#include <songs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <algorithm>

#include <SDL/SDL_image.h>

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

	FILE * fp = fopen(buff,"r");
	while(fgets(buff,256,fp)) {
		switch( buff[0] ) {
			case '#' :
				continue;
			case 'E' :
				break;
			case ':' :
			case '*' : {
				TNote * tmp = new TNote();
				int shift;
				int len = strlen(buff);
				char * syllable = new char[16];

				tmp->type = TYPE_NOTE_SING;
				buff[len-1] = '\0';
				sscanf(buff+1,"%d %d %d %n",&tmp->timestamp, &tmp->length , &tmp->note , &shift);
				sprintf(syllable,"%s",buff+shift+1);
				tmp->syllable = syllable;
				notes.push_back(tmp);
				break;
			}
			case '-' : {
				TNote * tmp = new TNote();
				tmp->type = TYPE_NOTE_SLEEP;
				sscanf(buff+1,"%d",&tmp->timestamp);
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
	video = NULL;
}

bool CSongs::parseFile( CSong * tmp )
{
	char buff[256];
	sprintf(buff,"%s/%s",tmp->path,tmp->filename);
	FILE * fp = fopen(buff,"r");
	if(!fp)
		return false;
	while(fgets(buff,256,fp)) {
		if(buff[0] != '#' )
			continue;
		if(!strncmp("#TITLE:",buff,7)) {
			int len = strlen(buff);
			char * title = new char[len - 7];

			buff[len-1]='\0'; // Replace the \n with a \0
			memcpy(title,buff+7,len - 7);
			tmp->title = title;
		} else if(!strncmp("#EDITION:",buff,9)) {
			int len = strlen(buff);
			char * edition = new char[len - 9];

			buff[len-1]='\0'; // Replace the \n with a \0
			memcpy(edition,buff+9,len - 9);
			tmp->edition = edition;
		} else if(!strncmp("#ARTIST:",buff,8)) {
			int len = strlen(buff);
			char * artist = new char[len - 8];

			buff[len-1]='\0'; // Replace the \n with a \0
			memcpy(artist,buff+8,len - 8);
			tmp->artist = artist;
		} else if(!strncmp("#MP3:",buff,5)) {
			int len = strlen(buff);
			char * mp3 = new char[len - 5];

			buff[len-1]='\0'; // Replace the \n with a \0
			memcpy(mp3,buff+5,len - 5);
			tmp->mp3 = mp3;
		} else if(!strncmp("#CREATOR:",buff,9)) {
			int len = strlen(buff);
			char * creator = new char[len - 9];

			buff[len-1]='\0'; // Replace the \n with a \0
			memcpy(creator,buff+9,len - 9);
			tmp->creator = creator;
		} else if(!strncmp("#GAP:",buff,5)) {
			sscanf(buff+5,"%f",&tmp->gap);
			// The following hack is to stay sync with ultratar
			tmp->gap+=200;
		} else if(!strncmp("#BPM:",buff,5)) {
			TBpm bpm;
			bpm.start = 0.0;
			sscanf(buff+5,"%f",&bpm.bpm);
			tmp->bpm.push_back(bpm);
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
		char * cover = new char[strlen(dirEntry->d_name)+4+1];
		sprintf(cover,"%s.png",dirEntry->d_name);
		tmp->cover = cover;

		sprintf(buff,"%s/%s/%s","songs",dirEntry->d_name,cover);
		SDL_RWops *rwop = SDL_RWFromFile(buff, "rb");
		SDL_Surface * coverSurface = IMG_LoadPNG_RW(rwop);
		if( coverSurface == NULL )
			tmp->coverSurf = surface_nocover;
		else {
			tmp->coverSurf = coverSurface;
			SDL_FreeRW(rwop);
		}
		if( !parseFile(tmp) ) {
			delete[] path;
			delete[] txt;
			delete[] cover;
			if( surface_nocover != tmp->coverSurf )
				delete tmp->coverSurf;
			delete tmp;
		} else {
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
		for(unsigned int j = 0; j < songs[i]->notes.size(); j++) {
			delete[] songs[i]->notes[j]->syllable;
			delete songs[i]->notes[j];
		}
		if( surface_nocover != songs[i]->coverSurf )
			SDL_FreeSurface(songs[i]->coverSurf);
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
