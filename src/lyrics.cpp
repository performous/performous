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
		formatedLyrics.push_back(tmp);
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
	return formatedLyrics[lastSentenceIndex];
}

TNote * CLyrics::getCurrentNote()
{
	return formatedLyrics[lastSentenceIndex][lastSyllableIndex];
}

void CLyrics::updateSentences( unsigned int timestamp )
{
	// If the sentences should change, lets re-compute it
	if( lastSyllableIndex == -1 || lastSentenceIndex == -1
		|| timestamp < getTimestampFromBeat( formatedLyrics[lastSentenceIndex][lastSyllableIndex]->timestamp )
		|| timestamp > getTimestampFromBeat( formatedLyrics[lastSentenceIndex][lastSyllableIndex]->timestamp + formatedLyrics[lastSentenceIndex][lastSyllableIndex]->length ) ) {
		unsigned int i;
		// If we are further than the last time (no rewind) (optimisation)
		if( lastSentenceIndex != -1 && timestamp > getTimestampFromBeat( formatedLyrics[lastSentenceIndex][0]->timestamp ) )
			i = lastSentenceIndex;
		else
			i = 0;
		// For all the detected sentences
		for(  ; i < formatedLyrics.size() ; i++ ) {
			// If we're localized in this sentence ( first_note < timestamp <= last_note+length )
			if( timestamp > getTimestampFromBeat( formatedLyrics[i][0]->timestamp )
				&& timestamp < getTimestampFromBeat( formatedLyrics[i][formatedLyrics[i].size()-1]->timestamp + formatedLyrics[i][formatedLyrics[i].size()-1]->length ) ) {
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
					if( i != formatedLyrics.size() - 1 ) {
						for( unsigned int j = 0 ; j < formatedLyrics[i+1].size() ; j++ ) {
							strcat( sentenceNext , formatedLyrics[i+1][j]->syllable );
						}
					}
					for( unsigned int j = 0 ; j < formatedLyrics[i].size() ; j++ ) {
						strcat( sentenceWhole , formatedLyrics[i][j]->syllable );
					}
				}
				// No need to go further in the song
				break;
			} else if( i != formatedLyrics.size() - 1 && timestamp < getTimestampFromBeat( formatedLyrics[i+1][0]->timestamp ) ) {
				sentencePast[0]   = '\0';
				sentenceNow[0]    = '\0';
				sentenceFuture[0] = '\0';
				sentenceNext[0]   = '\0';
				lastSentenceIndex = i;
				break;
				// If we're not before the first song
				if( i != 0 ) {
					for( unsigned int j = 0 ; j < formatedLyrics[i+1].size() ; j++ ) {
						strcat( sentenceFuture , formatedLyrics[i+1][j]->syllable );
					}
					if( i != formatedLyrics.size() - 2 ) {
						for( unsigned int j = 0 ; j < formatedLyrics[i+2].size() ; j++ ) {
							strcat( sentenceNext , formatedLyrics[i+2][j]->syllable );
						}
					}
				} else {
					for( unsigned int j = 0 ; j < formatedLyrics[0].size() ; j++ ) {
						strcat( sentenceFuture , formatedLyrics[0][j]->syllable );
					}
					strcat( sentenceWhole , sentenceFuture );
					for( unsigned int j = 0 ; j < formatedLyrics[1].size() ; j++ ) {
						strcat( sentenceNext , formatedLyrics[1][j]->syllable );
					}
				}
				break;
			}

		}
	}
}

unsigned int CLyrics::getTimestampFromBeat( unsigned int beat )
{
	return (unsigned int) ((beat * 60 * 1000) / ( bpm * 4 ) + gap);
}
