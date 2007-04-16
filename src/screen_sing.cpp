#include <screen_sing.h>
#include <songs.h>
#include <pitch_graph.h>
#include <cairotosdl.h>

CScreenSing::CScreenSing(char * name)
: pitchGraph(CScreenManager::getSingletonPtr()->getWidth(), CScreenManager::getSingletonPtr()->getHeight())
{
	screenName = name;
	play = false;
	finished = false;
	video = new CVideo();
	SDL_Surface *screen;
	previousFirstTimestamp = -1;

	CScreenManager * sm = CScreenManager::getSingletonPtr();
	screen = sm->getSDLScreen();

	videoSurf = SDL_AllocSurface( screen->flags,
			sm->getWidth(),
			sm->getHeight(),
			screen->format->BitsPerPixel,
			screen->format->Rmask,
			screen->format->Gmask,
			screen->format->Bmask,
			screen->format->Amask);
	SDL_SetAlpha(videoSurf, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
	//SDL_FillRect(videoSurf,NULL,0xffffff);
	backgroundSurf = SDL_AllocSurface( screen->flags,
			sm->getWidth(),
			sm->getHeight(),
			screen->format->BitsPerPixel,
			0x00ff0000,
			0x0000ff00,
			0x000000ff,
			0xff000000);
	SDL_SetAlpha(backgroundSurf, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
        SDL_FillRect(backgroundSurf,NULL,SDL_MapRGB(backgroundSurf->format, 255, 255, 255));
        theme = new CThemeSing();
}

CScreenSing::~CScreenSing()
{
	if(videoSurf)
            SDL_FreeSurface(videoSurf);
       	if(backgroundSurf)
            SDL_FreeSurface(backgroundSurf);
            
	delete video;
        delete theme;
}

void CScreenSing::manageEvent( SDL_Event event )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
        int keypressed;
	switch(event.type) {
		case SDL_KEYDOWN:
			keypressed = event.key.keysym.sym;
			if( keypressed == SDLK_ESCAPE || keypressed == SDLK_q ) {
				finished = true;
			} else if( keypressed == SDLK_SPACE || keypressed == SDLK_p ) {
				sm->getAudio()->togglePause();
			} else if( keypressed == SDLK_PLUS ) {
				playOffset += 20;
			} else if( keypressed == SDLK_MINUS ) {
				playOffset -= 20;
			} else if( keypressed == SDLK_LEFT ) {
				sm->getAudio()->seek(-5000);
			} else if( keypressed == SDLK_RIGHT ) {
				sm->getAudio()->seek(5000);
			} else if( keypressed == SDLK_UP ) {
				sm->getAudio()->seek(30000);
			} else if( keypressed == SDLK_DOWN ) {
				sm->getAudio()->seek(-30000);
			}
	}
	if( !play ) {
		char buff[1024];
		CSong * song = sm->getSong();
	        
		if( song->video != NULL ) {
			snprintf(buff,1024,"%s/%s",song->path,song->video);
			fprintf(stdout,"Now playing: (%d): %s\n",sm->getSongId(),buff);
			video->loadVideo(buff,videoSurf,sm->getWidth(),sm->getHeight());
                } else if ( song->background != NULL) {
                        SDL_BlitSurface(song->backgroundSurf,NULL,backgroundSurf,NULL);
                        SDL_BlitSurface(theme->bg->getSDLSurface(),NULL,backgroundSurf,NULL);
                        SDL_BlitSurface(theme->p1box->getSDLSurface(),NULL,backgroundSurf,NULL);
                } else {
                        SDL_FillRect(backgroundSurf,NULL,SDL_MapRGB(backgroundSurf->format, 255, 255, 255));
                        SDL_BlitSurface(theme->bg->getSDLSurface(),NULL,backgroundSurf,NULL);
                        SDL_BlitSurface(theme->p1box->getSDLSurface(),NULL,backgroundSurf,NULL);
                }
                backgroundSurf_id = sm->getVideoDriver()->initSurface(backgroundSurf);
		theme_id = sm->getVideoDriver()->initSurface(theme->theme->getCurrent());
                pitchGraph_id = sm->getVideoDriver()->initSurface(pitchGraph.getCurrent());
                snprintf(buff,1024,"%s/%s",song->path,song->mp3);
		fprintf(stdout,"Now playing: (%d): %s\n",sm->getSongId(),buff);
		sm->getAudio()->playMusic(buff);
		lyrics = new CLyrics( song->notes, song->gap, song->bpm[0].bpm );
		play = true;
	        song->score[0].score = 0;
		playOffset = 0;
        }
}

void CScreenSing::draw( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	CRecord * record    = sm->getRecord();
	CSong   * song      = sm->getSong();
        float freq;
	int note;

        theme->theme->clear();

	if( play && !sm->getAudio()->isPlaying() ) {
		finished = true;
	}

	if(finished) {
                sm->getAudio()->stopMusic();
		video->unloadVideo();
		SDL_FillRect(videoSurf,NULL,0xffffff);
		play = false;
		finished = false;
		sentence.clear();
		delete lyrics;
		pitchGraph.clear();
		sm->activateScreen("Songs");
	}

	//record->compute();
	freq = record->getFreq();
	note = record->getNoteId();


	// draw lines across the screen
	// Theme this
	unsigned int numOctaves = (song->noteMax+11)/12 - song->noteMin/12;
	unsigned int lowestC = (song->noteMin/12) * 12;  // the C below noteMin
	if( numOctaves < 3 ) numOctaves = 3;

	TThemeRect linerect;
	linerect.stroke_col.r = linerect.stroke_col.g = linerect.stroke_col.b = 0;
	linerect.stroke_col.a = 0.9;
	linerect.stroke_width = 1;
	linerect.svg_width = sm->getWidth();
	linerect.svg_height = sm->getHeight();
	linerect.height = 1;
	linerect.fill_col.a = 0.5;
	linerect.x = 0;
	linerect.width = sm->getWidth();
	linerect.fill_col.r = 50;
	linerect.fill_col.g = 50;
	linerect.fill_col.b = 50;
	// draw lines for the C notes (thick)
	for( unsigned int i = 0 ; i <= numOctaves ; i++ ) {
		if( i <= (song->noteMax-lowestC)/12 ) {
			linerect.y = sm->getHeight() * 3 / 4 - i * sm->getHeight() / 2 / numOctaves;
			theme->theme->DrawRect(linerect);
		}
	}
	linerect.stroke_width = 0;
	// draw the other lines in between
	for( unsigned int i = 0 ; i < numOctaves ; i++ ) {
		for( int j = 1 ; j < 12 ; j++ ) {
			if( i * 12 + j + (lowestC/12) * 12 <= (unsigned int)song->noteMax){
				linerect.y = sm->getHeight() * 3 / 4 - ( i * 12 + j ) * sm->getHeight() / 24 / numOctaves;
				theme->theme->DrawRect(linerect);
			}
		}
	}

	if(play) {
		// Get the time in the song
                unsigned int time = sm->getAudio()->getPosition();
		// Test is playOffset + time > 0
		if( playOffset < 0 && time < (unsigned int)(playOffset*-1)  )
			time = 0;
		else
			time += playOffset;

		double songPercent = (double)time / (double)sm->getAudio()->getLength();
		// Here we compute all about the lyrics
		lyrics->updateSentences( time );
		char * sentenceNextSentence = lyrics->getSentenceNext();
                char * sentencePast         = lyrics->getSentencePast();
		char * sentenceNow          = lyrics->getSentenceNow();
		char * sentenceFuture       = lyrics->getSentenceFuture();
                char * sentenceWhole        = lyrics->getSentenceWhole();
		sentence.clear();
		sentence = lyrics->getCurrentSentence();
		if( sentence.size() && previousFirstTimestamp != sentence[0]->timestamp ) {
			previousFirstTimestamp = sentence[0]->timestamp;
			pitchGraph.clear();
		}

		// Draw the video
		if( !video->isPlaying() && time > song->videoGap )
			video->play();

		if( video->isPlaying() ) {
	                /* FIXME: make video work with opengl, SMPEG sets alpha channel to zero */
                        SDL_BlitSurface(videoSurf,NULL,backgroundSurf,NULL);
                        sm->getVideoDriver()->drawSurface(backgroundSurf);
                        sm->getVideoDriver()->drawSurface(theme->bg->getSDLSurface());
		        sm->getVideoDriver()->drawSurface(theme->p1box->getSDLSurface());
		} else {
        		sm->getVideoDriver()->drawSurface(backgroundSurf_id);
                        sm->getVideoDriver()->updateSurface(backgroundSurf_id , (SDL_Surface *) NULL);
                }
                
		// Compute and draw the timer and the progressbar
		{
		char dateStr[32];
		sprintf(dateStr,"%.2u:%.2u",(time/1000)/60,(time/1000)%60);
                theme->timertxt.text = dateStr;
                theme->theme->PrintText(&theme->timertxt);
                theme->progressfg.width = theme->progressfg.final_width * songPercent;
                theme->theme->DrawRect(theme->progressfg); 
	        }
		//draw score		
                {
		char scoreStr[32];
                sprintf(scoreStr,"%04d",int(song->score[0].score/10)*10);
                theme->p1score.text = scoreStr;
                theme->theme->PrintText(&theme->p1score);
		}
                
		// draw the sang note TODO: themed sang note
                TThemeTxt tmptxt;
                {
		tmptxt = theme->timertxt;       // use timertxt as template
		tmptxt.text = record->getNoteStr(note);
		tmptxt.x=0;
		tmptxt.y=sm->getHeight();
		tmptxt.fontsize = 25;
                theme->theme->PrintText(&tmptxt);
                }

		// compute and draw the text
		unsigned int totalBpm;
		float bpmPixelUnit;
		if(sentence.size() ) {
        		totalBpm = sentence[sentence.size()-1]->length + sentence[sentence.size()-1]->timestamp - sentence[0]->timestamp;
			bpmPixelUnit = (sm->getWidth() - 100. - 100.)/(totalBpm*1.0);
		} else {
			totalBpm=0;
			bpmPixelUnit=0;
		}
		// Theme this
                TThemeRect tmprect;
                tmprect.stroke_col.r = tmprect.stroke_col.g = tmprect.stroke_col.b = 0;
		tmprect.stroke_col.a = 255;
		tmprect.stroke_width = 2;
                tmprect.svg_width = sm->getWidth();
                tmprect.svg_height = sm->getHeight();
                tmprect.height = 10;
		tmprect.fill_col.a = 255;

		// Compute and draw the "to start" cursor
		if (sentence.size()>0 && time < (sentence[0]->timestamp * 60 * 1000) / (song->bpm[0].bpm * 4 ) + song->gap){
			float waitLen = sentence[0]->timestamp - (time - song->gap) * (song->bpm[0].bpm * 4) / 60 / 1000;
			if( theme->tostartfg.final_height - waitLen * 5 < 0 )
				waitLen = theme->tostartfg.final_height;
			else
				waitLen = theme->tostartfg.final_height - waitLen * 5;
			
                	theme->tostartfg.height = theme->tostartfg.final_height - waitLen;
                	theme->theme->DrawRect(theme->tostartfg); 
		}

                for( unsigned int i = 0 ; i < sentence.size() ; i ++ ) {
		        int currentBpm = sentence[i]->timestamp - sentence[0]->timestamp;
		        int noteHeight=sm->getHeight()*3/4-((sentence[i]->note-lowestC)*sm->getHeight()/2/numOctaves/12);

		        // if C <= timestamp < N
		        if( time > ( (sentence[i]->timestamp+sentence[i]->length)  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap ) {
		        	int y = noteHeight;
		    		int begin = (int) (currentBpm*bpmPixelUnit);
		    		int end   = (int) ((currentBpm+sentence[i]->length)*bpmPixelUnit);
		    		tmprect.x = 105 + begin;
                                tmprect.y = y - 5;
                                tmprect.width = 100 + end - tmprect.x;
                                tmprect.fill_col.r = 0;
                                tmprect.fill_col.g = 0;
                                tmprect.fill_col.b = 255;
                                theme->theme->DrawRect(tmprect);
		        // if N+d <= timestamp < E
		        } else if( time < ( (sentence[i]->timestamp)  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap ) {
		        	int y = noteHeight;
		    		int begin = (int) (currentBpm*bpmPixelUnit);
		    		int end   = (int) ((currentBpm+sentence[i]->length)*bpmPixelUnit);
		    		tmprect.x = 105 + begin;
                                tmprect.y = y - 5;
                                tmprect.width = 100 + end - tmprect.x;
                                tmprect.fill_col.r = 200;
                                tmprect.fill_col.g = 200;
                                tmprect.fill_col.b = 200;
                                theme->theme->DrawRect(tmprect);
		        } else {
		    		int y = noteHeight;
		    		int begin   = (int) (currentBpm*bpmPixelUnit);
		    		int end     = (int) ((currentBpm+sentence[i]->length)*bpmPixelUnit);
		    		float note_start = (time - ( (sentence[i]->timestamp)  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) - song->gap);
		    		float note_total = (sentence[i]->length)  * 60 * 1000 / ( song->bpm[0].bpm * 4 );
		    		int current = (int) ((currentBpm + note_start*sentence[i]->length/note_total)*bpmPixelUnit);
		    		tmprect.x = 105 + begin;
                                tmprect.y = y - 5;
                                tmprect.width = 100 + current - tmprect.x;
                                tmprect.fill_col.r = 0;
                                tmprect.fill_col.g = 0;
                                tmprect.fill_col.b = 255;
                                theme->theme->DrawRect(tmprect);
		    	    
                                tmprect.x = 100 + current;
                                tmprect.y = y - 5;
                                tmprect.width = 100 + end - tmprect.x;
                                tmprect.fill_col.r = 200;
                                tmprect.fill_col.g = 200;
                                tmprect.fill_col.b = 200;
                                theme->theme->DrawRect(tmprect);
    	
		    	        // Lets find the nearest note from the song (diff in [-6,5])
		    	        int diff =  (66+sentence[i]->note - note)%12-6;
		    	        int noteSingFinal = sentence[i]->note - diff;
		    	        int noteheight=sm->getHeight()*3/4-((noteSingFinal-lowestC)*sm->getHeight()/2/numOctaves/12);
		    	        if(freq != 0.0) {
		    	        	pitchGraph.renderPitch(
		    					((float)noteheight/sm->getHeight()),
		    					((double)current + 100)/sm->getWidth());
		    	        	if( abs(diff) <= 2 - sm->getDifficulty() )
		    				song->score[0].score += (10000 / song->maxScore) * sentence[i]->type;
                                } else {
		    	      		pitchGraph.renderPitch( 0.0, ((double)current + 100)/sm->getWidth());
		    		}
		        }
		}
                
                tmptxt = theme->lyricspast;
                tmptxt.text = sentenceWhole;
                cairo_text_extents_t extents = theme->theme->GetTextExtents(tmptxt);
                theme->lyricspast.x = (theme->lyricspast.svg_width - extents.width)/2;
                theme->lyricspast.extents.x_advance = 0;
                theme->lyricshighlight.extents.x_advance= 0;
                
                if( sentencePast[0] ) {
                        theme->lyricspast.text = sentencePast;
                        theme->theme->PrintText(&theme->lyricspast);
		}
		
		if( sentenceNow[0] ) {
			unsigned int length = lyrics->getCurrentNote()->length;
			unsigned int timestamp = lyrics->getCurrentNote()->timestamp;
			float length_ms = length * 60 * 1000 / ( song->bpm[0].bpm * 4 );
			float timestamp_ms = timestamp * 60 * 1000 / ( song->bpm[0].bpm * 4 ) + song->gap;
			float started_ms = time - timestamp_ms;
			float factor = 1.2 - 0.2*started_ms/length_ms;

			if( factor < 1.0 ) factor = 1.0;
			if( factor > 1.2 ) factor = 1.2;
	                theme->lyricshighlight.x = theme->lyricspast.x + theme->lyricspast.extents.x_advance;
                        theme->lyricshighlight.text = sentenceNow;
                        theme->lyricshighlight.scale = factor;
                        theme->theme->PrintText(&theme->lyricshighlight);
 	        }
		
		if( sentenceFuture[0] ) {
	                theme->lyricsfuture.text = sentenceFuture;
                        theme->lyricsfuture.x = theme->lyricspast.x + theme->lyricspast.extents.x_advance + theme->lyricshighlight.extents.x_advance;
                        theme->theme->PrintText(&theme->lyricsfuture);
		}

	        if( sentenceNextSentence[0] ) {
                        theme->lyricsnextsentence.text = sentenceNextSentence;
                        theme->lyricsnextsentence.extents = theme->theme->GetTextExtents(theme->lyricsnextsentence);
                        theme->lyricsnextsentence.x = (theme->lyricsnextsentence.svg_width - theme->lyricsnextsentence.extents.width)/2;
                        theme->theme->PrintText(&theme->lyricsnextsentence);
                }

                sm->getVideoDriver()->updateSurface(theme_id, theme->theme->getCurrent());
		sm->getVideoDriver()->drawSurface(theme_id);
                sm->getVideoDriver()->updateSurface(pitchGraph_id, pitchGraph.getCurrent());
		sm->getVideoDriver()->drawSurface(pitchGraph_id);
	}
}
