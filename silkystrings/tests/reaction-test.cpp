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

#include <WM.h>
#include <GameView.h>
#include <Util.h>
#include <Reaction.h>
#include <Sound.h>

#include <cstdlib>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

int main()
{
  using namespace SilkyStrings;
  boost::shared_ptr<WM> wm (new WM (640, 480, false));
  boost::shared_ptr<GameView> view (new GameView(2.5, 0.15, wm));
  boost::shared_ptr<Sound> sound (new Sound());

  Reaction reaction(view, sound);

  bool hold = false;
  unsigned score = 0;
  double last_score = wm->get_clock();
  while (!wm->update_beginning_of_frame ())
  {

    view->update_beginning_of_frame (wm->get_clock ());


    if(hold){
      reaction.correctHold(wm->get_clock() - last_score);
      if(std::rand() % 100 == 0)
        hold = false;
    }


    last_score = wm->get_clock ();

    if (std::rand() % 100 == 0)
    {
      reaction.correctPick((double)std::rand()/(2*RAND_MAX) - 0.25); 
      hold = true;
    }

    if (std::rand() % 3000 == 0)
    {
      reaction.playWrong(); 
    }


    sound->update();
    sound->update();
    view->update_end_of_frame ();
    wm->update_end_of_frame ();
  }
}

