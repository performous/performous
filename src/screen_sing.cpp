#include <screen_sing.h>
#include <songs.h>
#include <pitch_graph.h>
#include <cairotosdl.h>

CScreenSing::CScreenSing(const char* name, unsigned int width, unsigned int height, Analyzer const& analyzer):
  CScreen(name,width,height), m_analyzer(analyzer), pitchGraph(width, height)
{
	video = new CVideo();
	SDL_Surface *screen;
	previousFirstTimestamp = -1;

	CScreenManager* sm = CScreenManager::getSingletonPtr();
	screen = sm->getSDLScreen();

	videoSurf = SDL_AllocSurface(screen->flags,
			width,
			height,
			screen->format->BitsPerPixel,
			screen->format->Rmask,
			screen->format->Gmask,
			screen->format->Bmask,
			screen->format->Amask);
	SDL_SetAlpha(videoSurf, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
	backgroundSurf = SDL_AllocSurface(screen->flags,
			width,
			height,
			screen->format->BitsPerPixel,
			0x00ff0000,
			0x0000ff00,
			0x000000ff,
			0xff000000);
	SDL_SetAlpha(backgroundSurf, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
	SDL_FillRect(backgroundSurf,NULL,SDL_MapRGB(backgroundSurf->format, 255, 255, 255));
}

CScreenSing::~CScreenSing() {
	if (videoSurf) SDL_FreeSurface(videoSurf);
	if (backgroundSurf) SDL_FreeSurface(backgroundSurf);
	delete video;
}

void CScreenSing::enter() {
	bool video_ok=false;
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	CSong& song = sm->getSongs()->current();
	theme = new CThemeSing(m_width, m_height);
	SDL_FillRect(backgroundSurf,NULL,SDL_MapRGB(backgroundSurf->format, 255, 255, 255));
	song.loadBackground();
	if (!song.video.empty()) {
		std::string file = song.path + "/" + song.video;
		std::cout << "Now playing: (" << sm->getSongs()->currentId() + 1 << "): " << file << std::endl;
		video_ok = video->loadVideo(file, videoSurf, m_width, m_height);
	}
	if (video_ok) {
		SDL_BlitSurface(theme->bg->getSDLSurface(),NULL,backgroundSurf,NULL);
		SDL_BlitSurface(theme->p1box->getSDLSurface(),NULL,backgroundSurf,NULL);
	} else if (song.backgroundSurf) {
		SDL_BlitSurface(song.backgroundSurf,NULL,backgroundSurf,NULL);
		SDL_BlitSurface(theme->bg->getSDLSurface(),NULL,backgroundSurf,NULL);
		SDL_BlitSurface(theme->p1box->getSDLSurface(),NULL,backgroundSurf,NULL);
	} else {
		SDL_BlitSurface(theme->bg->getSDLSurface(),NULL,backgroundSurf,NULL);
		SDL_BlitSurface(theme->p1box->getSDLSurface(),NULL,backgroundSurf,NULL);
	}
	backgroundSurf_id = sm->getVideoDriver()->initSurface(backgroundSurf);
	theme_id = sm->getVideoDriver()->initSurface(theme->theme->getCurrent());
	pitchGraph_id = sm->getVideoDriver()->initSurface(pitchGraph.getCurrent());
	std::string file = song.path + song.mp3;
	std::cout << "Now playing " << file << std::endl;
	sm->getAudio()->playMusic(file.c_str());
	lyrics = new CLyrics(song.notes, song.gap, song.bpm[0].bpm);
	song.score[0].score = 0;
	song.score[0].hits = 0;
	song.score[0].total = 0;
	playOffset = 0;
}

void CScreenSing::exit() {
	CScreenManager::getSingletonPtr()->getAudio()->stopMusic();
	video->unloadVideo();
	SDL_FillRect(videoSurf,NULL,0xffffff);
	m_sentence.clear();
	delete lyrics;
	delete theme;
	pitchGraph.clear();
}

void CScreenSing::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		CScreenManager* sm = CScreenManager::getSingletonPtr();
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->activateScreen("Score");
		else if (key == SDLK_SPACE || key == SDLK_p) sm->getAudio()->togglePause();
		else if (key == SDLK_PLUS) playOffset += 20;
		else if (key == SDLK_MINUS) playOffset -= 20;
		else if (key == SDLK_LEFT) sm->getAudio()->seek(-5000);
		else if (key == SDLK_RIGHT) sm->getAudio()->seek(5000);
		else if (key == SDLK_UP) sm->getAudio()->seek(30000);
		else if (key == SDLK_DOWN) sm->getAudio()->seek(-30000);
	}
}

void CScreenSing::draw() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (!sm->getAudio()->isPlaying()) {
		sm->activateScreen("Score");
		return;
	}
	CSong& song = sm->getSongs()->current();
	float freq = m_analyzer.getFreq();
	MusicalScale scale;
	int note = scale.getNoteId(freq);
	float resFactorX = m_width / 800.0;
	float resFactorY = m_height / 600.0;
	float resFactorAvg = (resFactorX + resFactorY) / 2.0;
	double oldfontsize;
	theme->theme->clear();
	// draw lines across the screen
	// Theme this
	unsigned int numOctaves = (song.noteMax+11)/12 - song.noteMin/12;
	unsigned int lowestC = (song.noteMin/12)* 12;  // the C below noteMin
	if (numOctaves < 3) numOctaves = 3;

	TThemeRect linerect;
	linerect.stroke_col.r = linerect.stroke_col.g = linerect.stroke_col.b = 0;
	linerect.stroke_col.a = 0.9;
	linerect.stroke_width = 1.*resFactorAvg;
	linerect.svg_width = m_width;
	linerect.svg_height = m_height;
	linerect.height = 1.*resFactorY;
	linerect.fill_col.a = 0.5;
	linerect.x = 0;
	linerect.width = m_width;
	linerect.fill_col.r = 50;
	linerect.fill_col.g = 50;
	linerect.fill_col.b = 50;
	linerect.final_height = 0;
	linerect.final_width  = 0;

	// draw lines for the C notes (thick)
	for(unsigned int i = 0 ; i <= numOctaves ; i++) {
		if (i <= (song.noteMax-lowestC)/12) {
			linerect.y = m_height* 3 / 4 - i* m_height / 2 / numOctaves;
			theme->theme->DrawRect(linerect);
		}
	}
	linerect.stroke_width = 0;
	// draw the other lines in between
	for(unsigned int i = 0 ; i < numOctaves ; i++) {
		for(int j = 1 ; j < 12 ; j++) {
			if (i* 12 + j + (lowestC/12)* 12 <= (unsigned int)song.noteMax){
				linerect.y = m_height* 3 / 4 - (i* 12 + j)* m_height / 24 / numOctaves;
				theme->theme->DrawRect(linerect);
			}
		}
	}
	// Get the time in the song
	unsigned int time = sm->getAudio()->getPosition();
	// Test is playOffset + time > 0
	if (playOffset < 0 && time < (unsigned int)(playOffset*-1)) time = 0;
	else time += playOffset;

	double songPercent = (double)time / (double)sm->getAudio()->getLength();
	// Here we compute all about the lyrics
	lyrics->updateSentences(time);
	std::string sentenceNextSentence = lyrics->getSentenceNext();
	std::string sentencePast = lyrics->getSentencePast();
	std::string sentenceNow = lyrics->getSentenceNow();
	std::string sentenceFuture = lyrics->getSentenceFuture();
	std::string sentenceWhole = lyrics->getSentenceWhole();
	m_sentence.clear();
	m_sentence = lyrics->getCurrentSentence();
	if (m_sentence.size() && previousFirstTimestamp != m_sentence[0].timestamp) {
		previousFirstTimestamp = m_sentence[0].timestamp;
		pitchGraph.clear();
	}
	// Draw the video
	if (!video->isPlaying() && time > song.videoGap) video->play();

	if (video->isPlaying()) {
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
		theme->progressfg.width = theme->progressfg.final_width* songPercent;
		theme->theme->DrawRect(theme->progressfg); 
	}
	//draw score		
	{
		char scoreStr[32];
		sprintf(scoreStr,"%04d",int(song.score[0].score/10)*10);
		theme->p1score.text = scoreStr;
		theme->theme->PrintText(&theme->p1score);
	}
	// draw the sang note TODO: themed sang note
	TThemeTxt tmptxt;
	{
		tmptxt = theme->timertxt;	   // use timertxt as template
		tmptxt.text = scale.getNoteStr(freq);
		tmptxt.x = 0;
		tmptxt.y = m_height;
		tmptxt.fontsize = 25;
		theme->theme->PrintText(&tmptxt);
	}
	// compute and draw the text
	unsigned int totalBpm;
	float bpmPixelUnit;
	if (m_sentence.size()) {
		TNote const& n = m_sentence[m_sentence.size()-1];
		totalBpm = n.length + n.timestamp - m_sentence[0].timestamp;
		bpmPixelUnit = (m_width - 100.*resFactorX - 100.*resFactorX)/(totalBpm * 1.0);
	} else {
		totalBpm = 0;
		bpmPixelUnit = 0;
	}
	// Theme this
	TThemeRect tmprect;
	tmprect.stroke_col.r = tmprect.stroke_col.g = tmprect.stroke_col.b = 0;
	tmprect.stroke_col.a = 255;
	tmprect.stroke_width = 2.*resFactorAvg;
	tmprect.svg_width = m_width;
	tmprect.svg_height = m_height;
	tmprect.height = 10.*resFactorY;
	tmprect.fill_col.a = 255;
	tmprect.final_height = 0;
	tmprect.final_width  = 0;
	// Compute and draw the "to start" cursor
	if (!m_sentence.empty() && time < (m_sentence[0].timestamp * 60 * 1000) / (song.bpm[0].bpm * 4) + song.gap){
		float waitLen = m_sentence[0].timestamp - (time - song.gap)* (song.bpm[0].bpm * 4) / 60 / 1000;
		if (theme->tostartfg.final_height - waitLen * 5 < 0)
		  waitLen = theme->tostartfg.final_height;
		else
		  waitLen = theme->tostartfg.final_height - waitLen * 5;
		theme->tostartfg.height = theme->tostartfg.final_height - waitLen;
		theme->theme->DrawRect(theme->tostartfg);
	}

	for (unsigned int i = 0; i < m_sentence.size(); ++i) {
		int currentBpm = m_sentence[i].timestamp - m_sentence[0].timestamp;
		int noteHeight=m_height*3/4-((m_sentence[i].note-lowestC)*m_height/2/numOctaves/12);
		// if C <= timestamp < N; note already ended
		if (time > ((m_sentence[i].timestamp+m_sentence[i].length) * 60 * 1000) / (song.bpm[0].bpm * 4) + song.gap) {
			int y = noteHeight;
			int begin = (int) (currentBpm *bpmPixelUnit);
			int end   = (int) ((currentBpm+m_sentence[i].length)*bpmPixelUnit);
			tmprect.x = 105.*resFactorX + begin;
			tmprect.y = y - 5.*resFactorY;
			tmprect.width = 100.*resFactorX + end - tmprect.x;
			tmprect.fill_col.r = 0;
			tmprect.fill_col.g = 0;
			tmprect.fill_col.b = 255;
			theme->theme->DrawRect(tmprect);
		// if N+d <= timestamp < E; note hasn't begun yet
		} else if (time < ((m_sentence[i].timestamp) * 60 * 1000) / (song.bpm[0].bpm * 4) + song.gap) {
			int y = noteHeight;
			int begin = (int) (currentBpm *bpmPixelUnit);
			int end   = (int) ((currentBpm+m_sentence[i].length)*bpmPixelUnit);
			tmprect.x = 105.*resFactorX + begin;
			tmprect.y = y - 5.*resFactorY;
			tmprect.width = 100.*resFactorX + end - tmprect.x;
			tmprect.fill_col.r = 200;
			tmprect.fill_col.g = 200;
			tmprect.fill_col.b = 200;
			theme->theme->DrawRect(tmprect);
		// note currently playing
		} else {
			int y = noteHeight;
			int begin   = (int) (currentBpm *bpmPixelUnit);
			int end	 = (int) ((currentBpm+m_sentence[i].length)*bpmPixelUnit);
			float note_start = (time - ((m_sentence[i].timestamp) * 60 * 1000) / (song.bpm[0].bpm * 4) - song.gap);
			float note_total = (m_sentence[i].length) * 60 * 1000 / (song.bpm[0].bpm * 4);
			int current = (int) ((currentBpm + note_start*m_sentence[i].length/note_total)*bpmPixelUnit);
			tmprect.x = 105.*resFactorX + begin;
			tmprect.y = y - 5.*resFactorY;
			tmprect.width = 100.*resFactorX + current - tmprect.x;
			tmprect.fill_col.r = 0;
			tmprect.fill_col.g = 0;
			tmprect.fill_col.b = 255;
			theme->theme->DrawRect(tmprect);
				
			tmprect.x = 100.*resFactorX + current;
			tmprect.y = y - 5.*resFactorY;
			tmprect.width = 100.*resFactorX + end - tmprect.x;
			tmprect.fill_col.r = 200;
			tmprect.fill_col.g = 200;
			tmprect.fill_col.b = 200;
			theme->theme->DrawRect(tmprect);

			float factor = ((float) m_sentence[i].curMaxScore)/song.maxScore;
			if (factor >= 0 && factor <= 1){
				song.score[0].score = (int)(10000*factor* song.score[0].hits/(song.score[0].total + 1));
			}
		}
	}

	if (!m_sentence.empty()) {
		double graphTime = (((time - song.gap) / 60000.0 - m_sentence[0].timestamp / (song.bpm[0].bpm * 4))* (song.bpm[0].bpm * 4)* bpmPixelUnit + 100.0) / sm->getWidth();
		if (freq == 0.0) {
			pitchGraph.renderPitch(0.0, graphTime);
		} else {
			unsigned int i = 0;
			// Find the currently playing note or the next playing note (or the last note?)
			while (i < m_sentence.size() && time > ((m_sentence[i].timestamp+m_sentence[i].length) * 60 * 1000) / (song.bpm[0].bpm * 4) + song.gap) ++i;
			// Lets find the nearest note from the song (diff in [-6,5])
			int diff =  (66+m_sentence[i].note - note)%12-6;
			int noteSingFinal = m_sentence[i].note - diff;
			int noteheight=((18*numOctaves-noteSingFinal+lowestC)*m_height/2/numOctaves/12);
			pitchGraph.renderPitch(double(noteheight) / m_height, graphTime);
			// Is the note currently playing?
			if (time >= ((m_sentence[i].timestamp) * 60 * 1000) / (song.bpm[0].bpm * 4) + song.gap &&
			  time <= ((m_sentence[i].timestamp+m_sentence[i].length) * 60 * 1000) / (song.bpm[0].bpm * 4) + song.gap) {
				song.score[0].total += 2;
				int error = abs(diff);
				if (error == 0) song.score[0].hits += 2;
				if (error == 1) song.score[0].hits += 1;
			}
		}
	}
	
	tmptxt = theme->lyricspast;
	tmptxt.text = sentenceWhole;
	{
		cairo_text_extents_t extents = theme->theme->GetTextExtents(tmptxt);
		theme->lyricspast.x = (theme->lyricspast.svg_width - extents.width)/2;
	}
	oldfontsize = theme->lyricspast.fontsize;
	while (theme->lyricspast.x < 0) {
		theme->lyricspast.fontsize -= 2;
		theme->lyricshighlight.fontsize -= 2;
		theme->lyricsfuture.fontsize -= 2;
		tmptxt = theme->lyricspast;
		tmptxt.text = sentenceWhole;
		cairo_text_extents_t extents = theme->theme->GetTextExtents(tmptxt);
		theme->lyricspast.x = (theme->lyricspast.svg_width - extents.width)/2;
	}
	theme->lyricspast.extents.x_advance = 0;
	theme->lyricshighlight.extents.x_advance= 0;
	
	if (sentencePast[0]) {
		theme->lyricspast.text = sentencePast;
		theme->theme->PrintText(&theme->lyricspast);
	}
	
	if (sentenceNow[0]) {
		unsigned int length = lyrics->getCurrentNote().length;
		unsigned int timestamp = lyrics->getCurrentNote().timestamp;
		float length_ms = length* 60 * 1000 / (song.bpm[0].bpm * 4);
		float timestamp_ms = timestamp* 60 * 1000 / (song.bpm[0].bpm * 4) + song.gap;
		float started_ms = time - timestamp_ms;
		float factor = 1.2 - 0.2*started_ms/length_ms;

		if (factor < 1.0) factor = 1.0;
		if (factor > 1.2) factor = 1.2;
		theme->lyricshighlight.x = theme->lyricspast.x + theme->lyricspast.extents.x_advance;
		theme->lyricshighlight.text = sentenceNow;
		theme->lyricshighlight.scale = factor;
		theme->theme->PrintText(&theme->lyricshighlight);
	}
	
	if (sentenceFuture[0]) {
		theme->lyricsfuture.text = sentenceFuture;
		theme->lyricsfuture.x = theme->lyricspast.x + theme->lyricspast.extents.x_advance + theme->lyricshighlight.extents.x_advance;
		theme->theme->PrintText(&theme->lyricsfuture);
	}

	if (sentenceNextSentence[0]) {
		theme->lyricsnextsentence.text = sentenceNextSentence;
		theme->lyricsnextsentence.extents = theme->theme->GetTextExtents(theme->lyricsnextsentence);
		theme->lyricsnextsentence.x = (theme->lyricsnextsentence.svg_width - theme->lyricsnextsentence.extents.width)/2;
		while (theme->lyricsnextsentence.x < 0) {
			theme->lyricsnextsentence.fontsize -= 2;
			theme->lyricsnextsentence.extents = theme->theme->GetTextExtents(theme->lyricsnextsentence);
			theme->lyricsnextsentence.x = (theme->lyricsnextsentence.svg_width - theme->lyricsnextsentence.extents.width)/2;
		}
		theme->theme->PrintText(&theme->lyricsnextsentence);
	}

	theme->lyricspast.fontsize = oldfontsize;
	theme->lyricshighlight.fontsize = oldfontsize;
	theme->lyricsfuture.fontsize = oldfontsize;
	theme->lyricsnextsentence.fontsize = oldfontsize;

	sm->getVideoDriver()->updateSurface(theme_id, theme->theme->getCurrent());
	sm->getVideoDriver()->drawSurface(theme_id);
	sm->getVideoDriver()->updateSurface(pitchGraph_id, pitchGraph.getCurrent());
	sm->getVideoDriver()->drawSurface(pitchGraph_id);
}
