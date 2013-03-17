#ifndef PLAYLIST_HH
#define PLAYLIST_HH

#include <iostream>
#include "song.hh"

struct listitem {
   Song* song;
   listitem * next;
 };

class playlist
{
private:
    listitem * pList = NULL;

public:
    playlist();
    void addToList(Song * songToAdd);
    bool isListEmpty();
    Song * getNextSongInQue();
};

#endif // PLAYLIST_HH
