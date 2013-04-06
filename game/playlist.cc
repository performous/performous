#include "playlist.hh"
#include "song.hh"

void PlayList::addSongToQue(boost::shared_ptr<Song> song)
{
   pList.push_back(song);
}
bool PlayList::isListEmpty()
{
  return pList.empty();
}
boost::shared_ptr<Song> PlayList::getNextSongInQueue()
{
   boost::shared_ptr<Song> nextSong;
   nextSong = pList[0];
   pList.erase(pList.begin());
   return nextSong;
}
