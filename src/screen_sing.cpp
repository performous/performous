#include <screen_sing.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_rotozoom.h>
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

	if( play && !sm->getAudio()->isPlaying() ) {
		play = false;
		CScreenManager::getSingletonPtr()->activateScreen("Songs");
		return;
	}

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
		SDL_Color white = {255,255,255,0};
		SDL_Color blue  = { 50, 50,255,0};
		// Declare the font we use
		TTF_Font *font;

		// Compute and draw the timer
		{
		char dateStr[32];
		sprintf(dateStr,"TIME");
		font = TTF_OpenFont("fonts/arial.ttf", 25);

		SDL_Surface * timeSurf1 = TTF_RenderUTF8_Blended(font, dateStr , white);
		boxRGBA( sm->getSDLScreen(),5 , 5 , 15+timeSurf1->w, 15+timeSurf1->h,0,0,0,255);
		position.x=10;
		position.y=10;
		SDL_BlitSurface(timeSurf1, NULL,  sm->getSDLScreen(), &position);

		sprintf(dateStr,"%.2u:%.2u",(time/1000)/60,(time/1000)%60);
		SDL_Surface * timeSurf2 = TTF_RenderUTF8_Blended(font, dateStr , black);
		position.x=timeSurf1->w+25;
		position.y=10;
		SDL_BlitSurface(timeSurf2, NULL,  sm->getSDLScreen(), &position);

		SDL_FreeSurface(timeSurf1);
		SDL_FreeSurface(timeSurf2);
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
			if( time > ( (song->notes[i]->timestamp + song->notes[i]->length )* 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap ) {
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
		char sentenceNow[128];
		char sentenceFuture[128];
		sentencePast[0] = '\0';
		sentenceNow[0] = '\0';
		sentenceFuture[0] = '\0';

		int pos = -1;
		for( i = currentSentence ; i < end ; i ++ ) {
			totalBpm+=song->notes[i]->length;
		}
		
		float bpmPixelUnit = (sm->getWidth() - 100. - 100.)/(totalBpm*1.0);
		int currentBpm = 0;

		for( i = currentSentence ; i < end ; i ++ ) {
			// if C <= timestamp < N
			if( time > ( (song->notes[i]->timestamp+song->notes[i]->length)  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap ) {
				int noteFinal = song->notes[i]->note - (song->noteMin/12)*12+(song->noteMin%12);
				int y = sm->getHeight()-(int)record->getNoteFreq(noteFinal);
				int begin = (int) (currentBpm*bpmPixelUnit);
				int end   = (int) ((currentBpm+song->notes[i]->length)*bpmPixelUnit);
				strcat(sentencePast,song->notes[i]->syllable);
				boxRGBA(sm->getSDLScreen(),105+begin,y-5,
			        	                   100+end,y+5,
			                	           0,0,255,255);
			}
			// if N+d <= timestamp < E
			else if( time < ( (song->notes[i]->timestamp)  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap ) {
				int noteFinal = song->notes[i]->note - (song->noteMin/12)*12+(song->noteMin%12);
				int y = sm->getHeight()-(int)record->getNoteFreq(noteFinal);
				int begin = (int) (currentBpm*bpmPixelUnit);
				int end   = (int) ((currentBpm+song->notes[i]->length)*bpmPixelUnit);
				strcat(sentenceFuture,song->notes[i]->syllable);
				boxRGBA(sm->getSDLScreen(),105+begin,y-5,
			        	                   100+end,y+5,
			                	           200,200,200,255);
			}
			else {
				strcat(sentenceNow,song->notes[i]->syllable);
				int noteFinal = song->notes[i]->note - (song->noteMin/12)*12+(song->noteMin%12);
				int y = sm->getHeight()-(int)record->getNoteFreq(noteFinal);
				int begin   = (int) (currentBpm*bpmPixelUnit);
				int end     = (int) ((currentBpm+song->notes[i]->length)*bpmPixelUnit);
				float note_start = (time - ( (song->notes[i]->timestamp)  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) - song->gap);
				float note_total = (song->notes[i]->length)  * 60 * 1000 / ( song->bpm[0].bpm * 4 );
				int current = (int) ((currentBpm + note_start*song->notes[i]->length/note_total)*bpmPixelUnit);
				boxRGBA(sm->getSDLScreen(),105+begin,y-5,
			        	                   100+current,y+5,
			                	           0,0,255,255);

				boxRGBA(sm->getSDLScreen(),100+current,y-5,
			        	                   100+end,y+5,
			                	           200,200,200,255);

				// Lets find the nearest note from the song
				int noteSingFinal = (note)%12;
				int noteFinal2    = (song->notes[i]->note)%12;
				int diff = abs(noteSingFinal-noteFinal2);
				if( diff > 6 )
					noteSingFinal = noteFinal - 12 + diff;
				else
					noteSingFinal = noteFinal + diff;
				if(freq != 0.0) {
					filledCircleRGBA(sm->getSDLScreen(),100+current, sm->getHeight()-(int)record->getNoteFreq(noteSingFinal),5,0,204,0,255);
				}
			}
			if( i != song->notes.size() - 1 &&
			    time > ( song->notes[i]->timestamp  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap &&
			    time < ( song->notes[i+1]->timestamp  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap &&
			 	song->notes[i]->type == TYPE_NOTE_SING )
				pos=i;
			currentBpm += song->notes[i]->length;
		}

		SDL_Surface * sentencePastSurf = NULL;
		SDL_Surface * sentenceFutureSurf = NULL;
		SDL_Surface * sentenceNowSurf = NULL;
		SDL_Surface * sentenceNowSurf2 = NULL;
		int sentencePastWidth = 0;
		int sentenceNowWidth = 0;
		int sentenceFutureWidth = 0;
		int sentenceWidth = 0;
		
		if( sentencePast[0] ) {
			sentencePastSurf = TTF_RenderUTF8_Blended(font, sentencePast , blue);
			sentencePastWidth = sentencePastSurf->w;
		}
		
		if( sentenceNow[0] && pos != -1) {
			sentenceNowSurf = TTF_RenderUTF8_Blended(font, sentenceNow , blue);
			unsigned int length = song->notes[pos]->length;
			unsigned int timestamp = song->notes[pos]->timestamp;
			float length_ms = length * 60 * 1000 / ( song->bpm[0].bpm * 4 );
			float timestamp_ms = timestamp * 60 * 1000 / ( song->bpm[0].bpm * 4 ) + song->gap;
			float started_ms = time - timestamp_ms;
			float factor = 1.2 - 0.2*started_ms/length_ms;

			sentenceNowSurf2 = zoomSurface (sentenceNowSurf, factor, factor, SMOOTHING_ON);
			sentenceNowWidth += sentenceNowSurf->w;
			SDL_FreeSurface(sentenceNowSurf);
		}
		
		if( sentenceFuture[0] ) {
			sentenceFutureSurf = TTF_RenderUTF8_Blended(font, sentenceFuture , black);
			sentenceFutureWidth = sentenceFutureSurf->w;
		}

		sentenceWidth = sentencePastWidth + sentenceNowWidth + sentenceFutureWidth;

		if( sentencePastSurf ) {
			position.x=(sm->getWidth()-sentenceWidth)/2;
			position.y=sm->getHeight()-sentencePastSurf->h;
			SDL_BlitSurface(sentencePastSurf, NULL,  sm->getSDLScreen(), &position);
			SDL_FreeSurface(sentencePastSurf);
		}
		
		if( sentenceNowSurf ) {
			position.x=(sm->getWidth()-sentenceWidth)/2+sentencePastWidth-(sentenceNowSurf2->w-sentenceNowWidth)/2;
			position.y=sm->getHeight()-sentenceNowSurf2->h;
			SDL_BlitSurface(sentenceNowSurf2, NULL,  sm->getSDLScreen(), &position);
			SDL_FreeSurface(sentenceNowSurf2);
		}
		
		if( sentenceFutureSurf ) {
			position.x=(sm->getWidth()-sentenceWidth)/2+sentencePastWidth+sentenceNowWidth;
			position.y=sm->getHeight()-sentenceFutureSurf->h;
			SDL_BlitSurface(sentenceFutureSurf, NULL,  sm->getSDLScreen(), &position);
			SDL_FreeSurface(sentenceFutureSurf);
		}

		TTF_CloseFont(font);


	}
}
