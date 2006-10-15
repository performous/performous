#include <screen_sing.h>
#include <SDL/SDL_ttf.h>
#include <songs.h>

CScreenSing::CScreenSing(char * name)
{
	screenName = name;
	play = false;
	finished = false;
	currentSentence = 0;
	currentSyllable = 0;

	SDL_Color black = {0, 0, 0,0};
	TTF_Font *font = TTF_OpenFont("fonts/arial.ttf", 65);
	SDL_Surface *title = TTF_RenderUTF8_Blended(font, "Let\'s Sing !!!", black);
	titleTex = new CSdlTexture(title);
	
	SDL_FreeSurface(title);
	TTF_CloseFont(font);
}

CScreenSing::~CScreenSing()
{
	delete titleTex;
}

void CScreenSing::manageEvent( SDL_Event event )
{
	int keypressed;
	switch(event.type) {
		case SDL_KEYDOWN:
			keypressed = event.key.keysym.sym;
			if( keypressed == SDLK_ESCAPE || keypressed == SDLK_q ) {
				CScreenManager::getSingletonPtr()->getAudio()->stopMusic();
				play = false;
				CScreenManager::getSingletonPtr()->activateScreen("Songs");
				return;
			}
	}
	if( !play ) {
		char buff[1024];
		currentSentence = 0;
		currentSyllable = 0;
		CSong * song = CScreenManager::getSingletonPtr()->getSong();
		sprintf(buff,"%s/%s",song->path,song->mp3);
		fprintf(stdout,"Now playing : (%d) : %s\n",CScreenManager::getSingletonPtr()->getSongId(),buff);
		CScreenManager::getSingletonPtr()->getAudio()->playMusic(buff);
		start = SDL_GetTicks();
		play = true;
	}
	
	if( finished ) {
		play = false;
		finished = false;
		currentSentence = 0;
		currentSyllable = 0;
		CScreenManager::getSingletonPtr()->activateScreen("Songs");
		return;
	}
		
}

void CScreenSing::draw( void )
{
	glColor4f(1.0,1.0,1.0,1.0);
	titleTex->draw( 200 , 0 , 400 , 100 );

	CScreenManager::getSingletonPtr()->getRecord()->compute();
	float freq = CScreenManager::getSingletonPtr()->getRecord()->getFreq();
	int note = CScreenManager::getSingletonPtr()->getRecord()->getNoteId();
	CSong * song = CScreenManager::getSingletonPtr()->getSong();

	if(play) {
		unsigned int date = SDL_GetTicks() - start;
		char dateStr[32];
		sprintf(dateStr,"Time: %u:%.2u",(date/1000)/60,(date/1000)%60);

		SDL_Color black = {  0,  0,  0,0};
		SDL_Color blue  = { 50, 50,255,0};
		TTF_Font *font = TTF_OpenFont("fonts/arial.ttf", 65);

		SDL_Surface * noteSurf = TTF_RenderUTF8_Blended(font, CScreenManager::getSingletonPtr()->getRecord()->getNoteStr(note) , black);
		CSdlTexture * noteTex = new CSdlTexture(noteSurf);
		noteTex->draw(300,450,100);
		SDL_FreeSurface(noteSurf);
		delete noteTex;
		TTF_CloseFont(font);

		font = TTF_OpenFont("fonts/arial.ttf", 25);
		SDL_Surface * timeSurf = TTF_RenderUTF8_Blended(font, dateStr , black);
		CSdlTexture * timeTex = new CSdlTexture(timeSurf);
		timeTex->draw(0,0);
		SDL_FreeSurface(timeSurf);
		delete timeTex;
		TTF_CloseFont(font);
		
		font = TTF_OpenFont("fonts/arial.ttf", 45);

		currentSentence = 0;
		unsigned int i = 0;

		for( i = 0 ; i <  song->notes.size() ; i++ ) {
			if( date > ( song->notes[i]->timestamp  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap &&
			    song->notes[i]->type == TYPE_NOTE_SLEEP ) {
				currentSentence=i;
			}
		}
		
		for( i = currentSentence ; i <  song->notes.size() ; i++ ) {
			if( song->notes[i]->type == TYPE_NOTE_SING ) {
				currentSentence = i;
				break;
			}
		}

		for( i = currentSentence ; i <  song->notes.size() ; i++ ) {
			if( song->notes[i]->type == TYPE_NOTE_SLEEP ) {
				break;
			}
		}

		unsigned int end = i;
		char sentencePast[128];
		char sentenceFuture[128];
		sentencePast[0] = '\0';
		sentenceFuture[0] = '\0';


		for( i = currentSentence ; i < end ; i ++ ) {
			if( date > ( song->notes[i]->timestamp  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap )
				strcat(sentencePast,song->notes[i]->syllable);
			else
				strcat(sentenceFuture,song->notes[i]->syllable);
		}
		
		SDL_Surface * sentencePastSurf = NULL;
		SDL_Surface * sentenceFutureSurf = NULL;
		CSdlTexture * sentencePastTex = NULL;
		CSdlTexture * sentenceFutureTex = NULL;
		int width = 0;
		int separation = 0;
		
		if( sentencePast[0] ) {
			sentencePastSurf = TTF_RenderUTF8_Blended(font, sentencePast , blue);
			sentencePastTex = new CSdlTexture(sentencePastSurf);
			width = sentencePastTex->getWidth();
			separation = width;
		}

		if( sentenceFuture[0] ) {
			sentenceFutureSurf = TTF_RenderUTF8_Blended(font, sentenceFuture , black);
			sentenceFutureTex = new CSdlTexture(sentenceFutureSurf);
			width += sentenceFutureTex->getWidth();
		}

		if( sentencePastTex )
			sentencePastTex->draw(400-(width/2),500);
		if( sentenceFutureTex )
			sentenceFutureTex->draw( 400-(width/2)+separation,500);


		if( sentencePastSurf ) {
			SDL_FreeSurface(sentencePastSurf);
			delete sentencePastTex;
		}

		if( sentenceFutureSurf ) {
			SDL_FreeSurface(sentenceFutureSurf);
			delete sentenceFutureTex;
		}

		int pos = -1;

		for( i = 0 ; i <  song->notes.size() - 1 ; i++ ) {
			 if( date > ( song->notes[i]->timestamp  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap &&
			     date < ( song->notes[i+1]->timestamp  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap &&
			 	song->notes[i]->type == TYPE_NOTE_SING )
				pos=i;
		}

		if( pos != -1 ) {
			glBegin(GL_POINTS);
				glColor4f(0.0,0.0,1.0,1.0);
				glVertex2d(400-(width/2)+separation, CScreenManager::getSingletonPtr()->getHeight()-(int)CScreenManager::getSingletonPtr()->getRecord()->getNoteFreq(song->notes[pos]->note));
			glEnd();
		}

		TTF_CloseFont(font);
	}

	if(freq != 0.0) {
		glPointSize(15.0);
		glBegin(GL_POINTS);
			glColor4f(0.6,0.0,0.0,1.0);
			glVertex2d(400, CScreenManager::getSingletonPtr()->getHeight()-(int)CScreenManager::getSingletonPtr()->getRecord()->getFreq());
			glColor4f(0.0,0.8,0.0,1.0);
			glVertex2d(400, CScreenManager::getSingletonPtr()->getHeight()-(int)CScreenManager::getSingletonPtr()->getRecord()->getNoteFreq(note));
		glEnd();
	}
}
