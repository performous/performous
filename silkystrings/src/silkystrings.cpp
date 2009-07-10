/* This file is part of SilkyStrings 
 * Copyright (C) 2006  Olli Salli, Tuomas Perälä, Ville Virkkala
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <iostream>
#include <sstream>
#include <string>
#include <exception>
#include <vector>

#include "Input.h"
#include "Key.h"
#include "Action.h"
#include "WM.h"
#include "GameView.h"


/**
 * Main program of SilkyStrings.
 * Parse commandline parameters, and initialize and start Action
 */
int main(int argc, char *argv[]){
 
  using namespace SilkyStrings;
  argc--;
  argv++;

  if(argc != 11) return -1;

  std::vector<std::istringstream> args (argv, argv + 11);

  int frets[5];
  int pick;
  int quit;
  int res_width;
  int res_height;
  std::string song;
  bool fullscreen;

  song = args[0].str ();
  for(int i=0; i<5; i++)
    args[1+i] >> frets[i];
  args[6] >> pick;
  args[7] >> quit;
  args[8] >> res_width;
  args[9] >> res_height;
  args[10] >> fullscreen;

  try
  {
  boost::shared_ptr<Input> in(new Input((Key)frets[0], (Key)frets[1], (Key)frets[2], (Key)frets[3], (Key)frets[4], (Key)pick, (Key)quit));
  boost::shared_ptr<WM> wm(new WM(res_width, res_height, fullscreen));
  boost::shared_ptr<GameView> view(new GameView(2.5, 0.075, wm));
  boost::shared_ptr<Sound> sound(new Sound);
  boost::shared_ptr<Reaction> reaction(new Reaction(view, sound));
  MidiFileParser parser(song + "notes.mid");
  Action action(wm,in,parser,view, reaction, sound, song);
  

  wm->register_client(in);

  action.start();
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught exception: " << e.what () << std::endl;
  }
  catch (...)
  {
    std::cerr << "Caught unknown exception!" << std::endl;
  }

  return 0;
}
