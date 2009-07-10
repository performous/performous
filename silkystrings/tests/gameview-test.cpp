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
#include <Input.h>
#include <Key.h>

#include <list>
#include <cstdlib>

#include <boost/lambda/lambda.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

namespace
{
  double frand ()
  {
    return double (std::rand ()) / double (RAND_MAX);
  }

  struct ChordCrack
  {
    unsigned fret;
    double start, end;
    SilkyStrings::GameView::ChordViewState state;
  };
}

int main()
{
  using namespace SilkyStrings;
  using namespace boost::lambda;
  boost::shared_ptr<WM> wm (new WM (640, 480, false));
  boost::shared_ptr<Input> input (new Input (KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_ENTER, KEY_ESC));
  GameView view (1.5, 0.075, wm);

  wm->register_client (input);

  unsigned score = 0;
  double last_score = 0.0;
  double last_popup = 0.0;

  std::list<ChordCrack> crack_lanes[5];

  while (!wm->update_beginning_of_frame ())
  {
    view.update_beginning_of_frame (wm->get_clock ());

    if (wm->get_clock () - last_score > 0.03f)
    {
      view.set_static_notification (boost::lexical_cast<std::string> (score++));

      last_score = wm->get_clock ();
    }

    if (wm->get_clock () - last_popup > 0.02f)
    {
      if (!(std::rand () % 69))
        view.set_popup_notification (boost::lexical_cast<std::string> (std::rand () % 50) + "x",
                                     0.3 + 0.2 * frand ());

      last_popup = wm->get_clock ();
    }

    for (int i = 0; i < 5; i++)
    {
      if (!crack_lanes[i].size ())
      {
        unsigned count = 1 + std::rand () % 3;
        double start = wm->get_clock () + 2.5 + 4 * frand ();

        for (int j = 0; j < count; j++)
        {
          double len = 1.5 + 1.5 * frand ();
          
          ChordCrack crack = {i + 1, start, start + len, GameView::ChordViewState (std::rand () % (GameView::CHORD_VIEW_STATE_FAILED + 1))};
          crack_lanes[i].push_back (crack);

          start += len;
        }
      }
      else
      {
        crack_lanes[i].remove_if ((&_1) ->* (&ChordCrack::end) < (wm->get_clock () - 5));
      }

      for (std::list<ChordCrack>::iterator iter = crack_lanes[i].begin ();
                                           iter != crack_lanes[i].end ();
                                           ++iter)
      {
        view.draw_chord (iter->fret, iter->start, iter->end, iter->state);
      }

      view.draw_fret (i + 1, input->fret_pressed (i + 1));
    }

    view.update_end_of_frame ();
    wm->update_end_of_frame ();
  }
}

