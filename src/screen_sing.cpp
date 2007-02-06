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

	screen = CScreenManager::getSingletonPtr()->getSDLScreen();

	videoSurf = SDL_AllocSurface( screen->flags,
			CScreenManager::getSingletonPtr()->getWidth(),
			CScreenManager::getSingletonPtr()->getHeight(),
			screen->format->BitsPerPixel,
			screen->format->Rmask,
			screen->format->Gmask,
			screen->format->Bmask,
			screen->format->Amask);
	SDL_FillRect(videoSurf,NULL,0xffffff);
        theme = new CThemeSing();
}

CScreenSing::~CScreenSing()
{
	if(videoSurf)
            SDL_FreeSurface(videoSurf);
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
			}
	}
	if( !play ) {
		char buff[1024];
		CSong * song = CScreenManager::getSingletonPtr()->getSong();
		
		if( song->video != NULL ) {
			snprintf(buff,1024,"%s/%s",song->path,song->video);
			fprintf(stdout,"Now playing: (%d) : %s\n",CScreenManager::getSingletonPtr()->getSongId(),buff);
			video->loadVideo(buff,videoSurf,sm->getWidth(),sm->getHeight());

                        if(song->backgroundSurf)
                                SDL_FreeSurface(song->backgroundSurf);
                        
	                static Uint32 rmask = 0x00ff0000;
                        static Uint32 gmask = 0x0000ff00;
                        static Uint32 bmask = 0x000000ff;
                        static Uint32 amask = 0xff000000;
	
	                song->backgroundSurf = SDL_CreateRGBSurfaceFrom((void *) theme->bg->getSDLSurface()->pixels,
		                                                        theme->bg->getSDLSurface()->w,
		                                                        theme->bg->getSDLSurface()->h,
		                                                        32, 
		                                                        theme->bg->getSDLSurface()->w*4, 
		                                                        rmask, gmask, bmask, amask);
	
	                SDL_SetAlpha(song->backgroundSurf, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);

                        SDL_BlitSurface(theme->bg->getSDLSurface(),NULL,song->backgroundSurf,NULL);
                        SDL_BlitSurface(theme->p1box->getSDLSurface(),NULL,song->backgroundSurf,NULL);
                        SDL_BlitSurface(song->backgroundSurf,NULL,sm->getSDLScreen(), NULL);
                } else if (song->backgroundSurf != NULL ) {
                        SDL_BlitSurface(theme->bg->getSDLSurface(),NULL,song->backgroundSurf,NULL);
                        SDL_BlitSurface(theme->p1box->getSDLSurface(),NULL,song->backgroundSurf,NULL);
                        SDL_BlitSurface(song->backgroundSurf,NULL,sm->getSDLScreen(), NULL);
               } else {
                        song->backgroundSurf = SDL_CreateRGBSurface( CScreenManager::getSingletonPtr()->getSDLScreen()->flags,
                                                        sm->getWidth() , sm->getHeight() ,
                                                        CScreenManager::getSingletonPtr()->getSDLScreen()->format->BitsPerPixel,
                                                        CScreenManager::getSingletonPtr()->getSDLScreen()->format->Rmask,
                                                        CScreenManager::getSingletonPtr()->getSDLScreen()->format->Gmask,
                                                        CScreenManager::getSingletonPtr()->getSDLScreen()->format->Bmask,
                                                        CScreenManager::getSingletonPtr()->getSDLScreen()->format->Amask);
                        SDL_FillRect(song->backgroundSurf,NULL,0xffffff);
                        SDL_BlitSurface(theme->bg->getSDLSurface(),NULL,song->backgroundSurf,NULL);
                        SDL_BlitSurface(theme->p1box->getSDLSurface(),NULL,song->backgroundSurf,NULL);
                        SDL_BlitSurface(song->backgroundSurf,NULL,sm->getSDLScreen(), NULL);

               }


		snprintf(buff,1024,"%s/%s",song->path,song->mp3);
		fprintf(stdout,"Now playing : (%d) : %s\n",CScreenManager::getSingletonPtr()->getSongId(),buff);
		CScreenManager::getSingletonPtr()->getAudio()->playMusic(buff);
		sentenceNextSentence[0] = '\n';
                start = SDL_GetTicks();
		play = true;
	        song->score[0].score = 0;
                song_pos = 0;
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

        theme->theme->clear();

	if( play && !sm->getAudio()->isPlaying() ) {
		finished = true;
	}

	if(finished) {
                CScreenManager::getSingletonPtr()->getAudio()->stopMusic();
		video->unloadVideo();
		SDL_FillRect(videoSurf,NULL,0xffffff);
		play = false;
		finished = false;
		sentence.clear();
		pitchGraph.clear();
		CScreenManager::getSingletonPtr()->activateScreen("Songs");
	}

	//record->compute();
	freq = record->getFreq();
	note = record->getNoteId();

	if(play) {
		// Get the time in the song
                unsigned int time = sm->getAudio()->getPosition();
		// Compute how far we're in the song
		double songPercent = (double)time / (double)sm->getAudio()->getLength();

		// Draw the video
		if( !video->isPlaying() && time > song->videoGap )
			video->play();
		if( video->isPlaying() ) {
			position.x=0;
			position.y=0;
			SDL_BlitSurface(videoSurf, NULL,  sm->getSDLScreen(), &position);
		}

                if (song->backgroundSurf) {
                    SDL_BlitSurface(song->backgroundSurf,NULL,sm->getSDLScreen(), NULL);
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
		unsigned int i = 0;
		unsigned int totalBpm = 0;

		// If the sentence is not empty and the last syllable is in the past
		// we clear the sentence
		if( !sentence.empty() &&
		    time > ( (sentence[sentence.size()-1]->timestamp + sentence[sentence.size()-1]->length )* 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap ) {
                        sentence.clear();
                        pitchGraph.clear();
		}
		// If the sentence is empty we create it
                if( sentence.empty() ) {
                        while(song->notes[song_pos]->type == TYPE_NOTE_SLEEP) {
                            if (song_pos < song->notes.size() - 1)
                                song_pos++;
                            else
                                break;
                        }
                        while(song->notes[song_pos]->type != TYPE_NOTE_SLEEP) {
                            sentence.push_back(song->notes[song_pos]);
                            if (song_pos < song->notes.size() - 1)
                                song_pos++;
                            else 
                                break;
                        }
                        sentenceNextSentence[0] = '\0';
                        i = song_pos;
                        /* Load the next sentence */
                        if (i < song->notes.size() - 1) {
                            while(song->notes[i]->type == TYPE_NOTE_SLEEP) {
                                if(i < song->notes.size() - 1)
                                    i++;
                                else
                                    break;
                            }
                            while(song->notes[i]->type != TYPE_NOTE_SLEEP) {
                                strcat(sentenceNextSentence, song->notes[i]->syllable); 
                                if(i < song->notes.size() - 1)
                                    i++;
                                else
                                    break;
                            }
                        }
                }
		
                char sentencePast[128]   ; sentencePast[0]   = '\0';
		char sentenceNow[128]    ; sentenceNow[0]    = '\0';
		char sentenceFuture[128] ; sentenceFuture[0] = '\0';
                char sentenceWhole[128]; sentenceWhole[0] = '\0';
        	totalBpm = sentence[sentence.size()-1]->length + sentence[sentence.size()-1]->timestamp - sentence[0]->timestamp;
		
		float bpmPixelUnit = (sm->getWidth() - 100. - 100.)/(totalBpm*1.0);

		int pos = -1;
                if (time <= ((song->notes[song->notes.size() - 1]->timestamp + song->notes[song->notes.size()-1]->length)*60*1000) / ( song->bpm[0].bpm * 4 ) + song->gap) {
                    for( i = 0 ; i < sentence.size() ; i ++ ) {
			    int currentBpm = sentence[i]->timestamp - sentence[0]->timestamp;
			    // if C <= timestamp < N
			    if( time > ( (sentence[i]->timestamp+sentence[i]->length)  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap ) {
			            int noteFinal = sentence[i]->note - (song->noteMin/12)*12;
				    int y = sm->getHeight()-(int)record->getNoteFreq(noteFinal);
				    int begin = (int) (currentBpm*bpmPixelUnit);
				    int end   = (int) ((currentBpm+sentence[i]->length)*bpmPixelUnit);
				    strcat(sentencePast,sentence[i]->syllable);
				    boxRGBA(sm->getSDLScreen(),105+begin,y-5,
			                	                   100+end,y+5,
			                        	           0,0,255,255);
			    }
			    // if N+d <= timestamp < E
			    else if( time < ( (sentence[i]->timestamp)  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap ) {
				    int noteFinal = sentence[i]->note - (song->noteMin/12)*12;
				    int y = sm->getHeight()-(int)record->getNoteFreq(noteFinal);
				    int begin = (int) (currentBpm*bpmPixelUnit);
				    int end   = (int) ((currentBpm+sentence[i]->length)*bpmPixelUnit);
				    strcat(sentenceFuture,sentence[i]->syllable);
				    boxRGBA(sm->getSDLScreen(),105+begin,y-5,
			                	                   100+end,y+5,
			                        	           200,200,200,255);
			    }
			    else {
				    strcat(sentenceNow,sentence[i]->syllable);
				    int noteFinal = sentence[i]->note - (song->noteMin/12)*12;
				    int y = sm->getHeight()-(int)record->getNoteFreq(noteFinal);
				    int begin   = (int) (currentBpm*bpmPixelUnit);
				    int end     = (int) ((currentBpm+sentence[i]->length)*bpmPixelUnit);
				    float note_start = (time - ( (sentence[i]->timestamp)  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) - song->gap);
				    float note_total = (sentence[i]->length)  * 60 * 1000 / ( song->bpm[0].bpm * 4 );
				    int current = (int) ((currentBpm + note_start*sentence[i]->length/note_total)*bpmPixelUnit);
				    boxRGBA(sm->getSDLScreen(),105+begin,y-5,
			        	                       100+current,y+5,
			                        	       0,0,255,255);

				    boxRGBA(sm->getSDLScreen(),100+current,y-5,
			        	                       100+end,y+5,
			                	               200,200,200,255);

				    // Lets find the nearest note from the song
				    int diff =  sentence[i]->note%12 - note%12;

				    if( diff > 6 )
					    diff -= 12;
				    if( diff < -6 )
					    diff += 12;

				    int noteSingFinal = noteFinal - diff ;
				    if( noteSingFinal < 0 )
					    noteSingFinal +=12;

				    if(freq != 0.0) {
					    pitchGraph.renderPitch(
						    (sm->getHeight() - record->getNoteFreq(noteSingFinal))/sm->getHeight(),
						    ((double)current)/sm->getWidth());
                                                    if (record->getNoteFreq(noteSingFinal) == record->getNoteFreq(noteFinal)) {
                                                        song->score[0].score += (10000 / song->maxScore) * sentence[i]->type;
                                                    }
                                    } else {
					    pitchGraph.renderPitch(
						    0.0,
						    ((double)current)/sm->getWidth());
				    }
				    if( time < ( (sentence[i]->timestamp+sentence[i]->length)  * 60 * 1000) / ( song->bpm[0].bpm * 4 ) + song->gap )
					    pos=i;
			    }
                            strcat(sentenceWhole, sentence[i]->syllable);
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
		
		if( sentenceNow[0] && pos != -1) {
			unsigned int length = sentence[pos]->length;
			unsigned int timestamp = sentence[pos]->timestamp;
			float length_ms = length * 60 * 1000 / ( song->bpm[0].bpm * 4 );
			float timestamp_ms = timestamp * 60 * 1000 / ( song->bpm[0].bpm * 4 ) + song->gap;
			float started_ms = time - timestamp_ms;
			float factor = 1.2 - 0.2*started_ms/length_ms;

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
		SDL_Surface* pitchGraphSurf = CairoToSdl::BlitToSdl(pitchGraph.getCurrent());
		position.x = 100;
		position.y = 0;
		SDL_BlitSurface(pitchGraphSurf,	NULL, sm->getSDLScreen(), &position);
		SDL_FreeSurface(pitchGraphSurf);

                SDL_Surface *themeSurf = CairoToSdl::BlitToSdl(theme->theme->getCurrent());
                SDL_BlitSurface(themeSurf, NULL,sm->getSDLScreen(),NULL);
                SDL_FreeSurface(themeSurf);
	}
 
}
