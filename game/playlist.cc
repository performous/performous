#include "playlist.hh"
#include "song.hh"

playlist::playlist()
{
}

void playlist::addToList(Song * songToAdd)
{
    listitem * itemToAdd;
    itemToAdd->song = songToAdd;
    itemToAdd->next = NULL; // just to make sure it really is NULL
    if(pList == nullptr) // if the ist is empty
       pList = itemToAdd;
    else
    {
    listitem * cyclePtr = pList;
    while(cyclePtr->next != NULL) //get to the end of the list
    {
       cyclePtr = cyclePtr->next;
    }
    cyclePtr->next = itemToAdd; //and add song to end of que
    }
}

bool playlist::isListEmpty()
{
    return pList == NULL;
}

Song * playlist::getNextSongInQue()
{
    if(pList == NULL)
        return NULL;

    Song * songToPlay;
    listitem * secondSongInQue = pList->next;
    songToPlay = pList->song;
    delete pList;
    pList = secondSongInQue;
    return songToPlay;
}
