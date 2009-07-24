#pragma once

#include "player.hh"

/**A collection of all Players.
 
 The current players plugged in a song can
 be retrieved with Engine::getPlayers().*/
class Players
{
  private:
	std::list<Player> m_players;
  public:
	void load();
	void save();
};
