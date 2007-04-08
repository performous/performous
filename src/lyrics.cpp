#include <lyrics.h>

CLyrics::CLyrics( std::vector <TNote *> _lyrics , float _gap , float _bpm )
{
	gap = _gap;
	bpm = _bpm;
	lyrics            = _lyrics;
	lastSyllableIndex = -1;
	lastSentenceIndex = -1;
	sentencePast[0]   = '\0';
	sentenceNow[0]    = '\0';
	sentenceFuture[0] = '\0';
	sentenceNext[0]   = '\0';
	unsigned int nbSyllable = lyrics.size();
	std::vector <TNote *> tmp;
	for( unsigned int i = 0 ; i < nbSyllable ; i++ ) {
		while(i < nbSyllable && lyrics[i]->type == TYPE_NOTE_SLEEP) {
			i++;
		}
		while(i < nbSyllable && lyrics[i]->type != TYPE_NOTE_SLEEP) {
			tmp.push_back(lyrics[i]);
			i++;
		}
		if(tmp.size()) {
			formatedLyrics.push_back(tmp);
		}
		tmp.clear();
	}
}

CLyrics::~CLyrics( )
{
	lyrics.clear();
	formatedLyrics.clear();
}

char * CLyrics::getSentencePast()
{
	return sentencePast;
}

char * CLyrics::getSentenceNow()
{
	return sentenceNow;
}

char * CLyrics::getSentenceFuture()
{
	return sentenceFuture;
}

char * CLyrics::getSentenceNext()
{
	return sentenceNext;
}

char * CLyrics::getSentenceWhole()
{
	return sentenceWhole;
}

std::vector <TNote *> CLyrics::getCurrentSentence()
{
	if( lastSentenceIndex != -1 ) {
		return formatedLyrics[lastSentenceIndex];
	} else {
		std::vector <TNote *> tmp;
		return tmp;
	}
}

TNote * CLyrics::getCurrentNote()
{
	if( lastSentenceIndex != -1 && lastSyllableIndex != -1 )
		return formatedLyrics[lastSentenceIndex][lastSyllableIndex];
	else
		return NULL;
}

void CLyrics::updateSentences( unsigned int timestamp )
{
	// If the sentences shouldn't change, do nothing
	if( lastSyllableIndex != -1 && lastSentenceIndex != -1
	    && lastSentenceIndex < (int) formatedLyrics.size()
	    && lastSyllableIndex < (int) formatedLyrics[lastSentenceIndex].size()
	    && timestamp >= getTimestampFromBeat( formatedLyrics[lastSentenceIndex][lastSyllableIndex]->timestamp )
	    && timestamp <= getTimestampFromBeat( formatedLyrics[lastSentenceIndex][lastSyllableIndex]->timestamp + formatedLyrics[lastSentenceIndex][lastSyllableIndex]->length ) ) {
		return;
	}
	// sentence changed, recompute it
	unsigned int i;
	
	// If we are further than the last time (no rewind) (optimisation)
	if( lastSentenceIndex != -1 && timestamp > getStartTime(lastSentenceIndex))
		i = lastSentenceIndex;
	else
		i = 0;

	// For all the detected sentences, find the first that have not yet ended
	for(  ; i < formatedLyrics.size() ; i++ ) {
		// If we're between the end of the last sentence and the end of the current sentence
		if( timestamp > getEndTime(i-1) && timestamp <= getEndTime(i) ) {
			sentencePast[0]   = '\0';
			sentenceNow[0]    = '\0';
			sentenceFuture[0] = '\0';
			for( unsigned int j = 0 ; j < formatedLyrics[i].size() ; j++ ) {
				if( timestamp > getTimestampFromBeat( formatedLyrics[i][j]->timestamp + formatedLyrics[i][j]->length ) ) {
					strcat( sentencePast , formatedLyrics[i][j]->syllable );
				} else if( timestamp < getTimestampFromBeat( formatedLyrics[i][j]->timestamp ) ) {
					strcat( sentenceFuture , formatedLyrics[i][j]->syllable );
				} else {
					lastSyllableIndex = j;
					strcat( sentenceNow , formatedLyrics[i][j]->syllable );
				}
			}
			// If we have change of sentence, we rebuild the next sentence
			if( lastSentenceIndex == -1 || (unsigned int)lastSentenceIndex != i ) {
				lastSentenceIndex = i;
				sentenceNext[0]   = '\0';
				sentenceWhole[0]  = '\0';
				if( i != formatedLyrics.size() - 1 )
					for( unsigned int j = 0 ; j < formatedLyrics[i+1].size() ; j++ )
						strcat( sentenceNext , formatedLyrics[i+1][j]->syllable );
				for( unsigned int j = 0 ; j < formatedLyrics[i].size() ; j++ )
					strcat( sentenceWhole , formatedLyrics[i][j]->syllable );
			}
			// No need to go further in the song
			break;
		}
	}

}

unsigned int CLyrics::getTimestampFromBeat( unsigned int beat )
{
	return (unsigned int) ((beat * 60 * 1000) / ( bpm * 4 ) + gap);
}

unsigned int CLyrics::getStartTime( int sentence )
{
	if( sentence < 0 )
		return 0;
	if( (unsigned int) sentence >= formatedLyrics.size() )
		return UINT_MAX;
	return getTimestampFromBeat( formatedLyrics[sentence][0]->timestamp );
}

unsigned int CLyrics::getEndTime( int sentence )
{
	if( sentence < 0 )
		return 0;
	if( (unsigned int) sentence >= formatedLyrics.size() )
		return UINT_MAX;
	return getTimestampFromBeat( formatedLyrics[sentence][formatedLyrics[sentence].size()-1]->timestamp + formatedLyrics[sentence][formatedLyrics[sentence].size()-1]->length );
}
