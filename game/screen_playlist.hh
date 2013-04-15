#pragma once

#include "screen.hh"

class screen_playlist : public Screen
{
public:
  screen_playlist();
  void manageEvent(input::NavEvent const& event) = 0;
  void manageEvent(SDL_Event);
  void prepare();
  void draw();
  void enter();
  void exit() = 0;
  void reloadGL();
private:
};


