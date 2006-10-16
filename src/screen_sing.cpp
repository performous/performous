#include <screen_sing.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <songs.h>

CScreenSing::CScreenSing(char * name)
{
	screenName = name;
	play = false;
	finished = false;

	SDL_Color black = {0, 0, 0,0};
	TTF_Font *font = TTF_OpenFont("fonts/arial.ttf", 65);
	title = TTF_RenderUTF8_Blended(font, "Let\'s Sing !!!", black);
	
	TTF_CloseFont(font);
}

CScreenSing::~CScreenSing()
{
	SDL_FreeSurface(title);
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
		CScreenManager::getSingletonPtr()->activateScreen("Songs");
		return;
	}
		
}

void CScreenSing::draw( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	CRecord * record    = sm->getRecord();
	CSong   * song      = sm->getSong();
	SDL_Rect position;
	float freq;
	int note;

	// Draw the title
	position.x=(sm->getWidth()-title->w)/2;
	position.y=0;
	SDL_BlitSurface(title, NULL,  sm->getSDLScreen(), &position);


	//record->compute();
	freq = record->getFreq();
	note = record->getNoteId();

	if(play) {
		// Compute the time in the song
		unsigned int time = SDL_GetTicks() - start;
		// Load usefull colors
		SDL_Color black = {  0,  0,  0,0};
		SDL_Color blue  = { 50, 50,255,0};
		// Declare the font we use
		TTF_Font *font;

		// Compute and draw the timer
		{
		char dateStr[32];
		sprintf(dateStr,"Time: %u:%.2u",(time/1000)/60,(time/1000)%60);
		font = TTF_OpenFont("fonts/arial.ttf", 25);
		SDL_Surface * timeSurf = TTF_RenderUTF8_Blended(font, dateStr , black);
		position.x=0;
		position.y=0;
		SDL_BlitSurface(timeSurf, NULL,  sm->getSDLScreen(), &position);
		SDL_FreeSurface(timeSurf);
		TTF_CloseFont(font);
		}
		
		// draw the sang note
		{
		font = TTF_OpenFont("fonts/arial.ttf", 25);
		SDL_Surface * noteSurf = TTF_RenderUTF8_Blended(font, record->getNoteStr(note) , black);
		position.x=0;
		position.y=sm->getHeight()-noteSurf->h;
		SDL_BlitSurface(noteSurf, NULL,  sm->getSDLScreen(), &position);
		SDL_FreeSurface(noteSurf);
		TTF_CloseFont(font);
		}

		// compute and draw the text
		font = TTF_OpenFont("fonts/arial.ttf", 40);
		unsigned int currentSentence = 0;
		unsigned int i = 0;
		unsigned int end = 0;
		unsigned int totalBpm = 0;

		// Find the last SLEEP before where we are
		for( i = 0 ; i <  song->notes.size() ; i++ ) {
			totalBpm += song->notes[i]->length;
			if( time > ( song->notes[i]->timestamp  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap ) {
				if( song->notes[i]->type == TYPE_NOTE_SLEEP )
					currentSentence=i;
			} else {
				// Here we are too far
				break;
			}
		}
		
		// Find the first SING after this SLEEP and the next SLEEP
		bool found=false;
		for( end = currentSentence ; end <  song->notes.size() ; end++ ) {
			if( !found && song->notes[end]->type == TYPE_NOTE_SING ) {
				currentSentence = end;
				found = true;
			}
			if( found && song->notes[end]->type == TYPE_NOTE_SLEEP )
				break;
		}

		// Here we have : (L: SLEEP, I: SING)
		// N: now
		// C: currentSentence
		// E: end
		//
		// L  I  I  I  I  I  I  L
		//    |      |          |
		//    C      N          E

		char sentencePast[128];
		char sentenceFuture[128];
		sentencePast[0] = '\0';
		sentenceFuture[0] = '\0';

		
		for( i = currentSentence ; i < end ; i ++ ) {
			// if C <= timestamp < N
			if( time > ( song->notes[i]->timestamp  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap )
				strcat(sentencePast,song->notes[i]->syllable);
			// if N <= timestamp < E
			else
				strcat(sentenceFuture,song->notes[i]->syllable);
		}

		SDL_Surface * sentencePastSurf = NULL;
		SDL_Surface * sentenceFutureSurf = NULL;
		int sentenceSurfWidth = 0;
		int separation = 0;
		
		if( sentencePast[0] ) {
			sentencePastSurf = TTF_RenderUTF8_Blended(font, sentencePast , blue);
			sentenceSurfWidth = sentencePastSurf->w;
			separation = sentencePastSurf->w;
		}

		if( sentenceFuture[0] ) {
			sentenceFutureSurf = TTF_RenderUTF8_Blended(font, sentenceFuture , black);
			sentenceSurfWidth += sentenceFutureSurf->w;
		}

		if( sentencePastSurf ) {
			position.x=(sm->getWidth()-sentenceSurfWidth)/2;
			position.y=550;
			SDL_BlitSurface(sentencePastSurf, NULL,  sm->getSDLScreen(), &position);
			SDL_FreeSurface(sentencePastSurf);
		}
		if( sentenceFutureSurf ) {
			position.x=(sm->getWidth()-sentenceSurfWidth)/2+separation;
			position.y=550;
			SDL_BlitSurface(sentenceFutureSurf, NULL,  sm->getSDLScreen(), &position);
			SDL_FreeSurface(sentenceFutureSurf);
		}

		TTF_CloseFont(font);

		int pos = -1;

		for( i = 0 ; i <  song->notes.size() - 1 ; i++ ) {
			 if( time > ( song->notes[i]->timestamp  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap &&
			     time < ( song->notes[i+1]->timestamp  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap &&
			 	song->notes[i]->type == TYPE_NOTE_SING )
				pos=i;
		}

		if( pos != -1 ) {
			filledCircleRGBA(sm->getSDLScreen(),(sm->getWidth()-sentenceSurfWidth)/2+separation, sm->getHeight()-(int)record->getNoteFreq(song->notes[pos]->note),5,0,0,255,255);
		}

		if(freq != 0.0) {
			filledCircleRGBA(sm->getSDLScreen(),(sm->getWidth()-sentenceSurfWidth)/2+separation, sm->getHeight()-(int)record->getFreq(),5,153,0,0,255);
			filledCircleRGBA(sm->getSDLScreen(),(sm->getWidth()-sentenceSurfWidth)/2+separation, sm->getHeight()-(int)record->getNoteFreq(note),5,0,204,0,255);
		}
	}
}
