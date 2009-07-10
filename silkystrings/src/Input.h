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

#ifndef __SS_INPUT_H_
#define __SS_INPUT_H_

#include "KeyEventClient.h"
#include "Key.h"

namespace SilkyStrings
{
  /**
   * The Input class translates physical input events to game events.
   */

  class Input : public KeyEventClient
  {
    public:

      /** Constructor.
       *
       * Constructs a Input object. All the keys in a newly created object are
       * in the unpressed state.
       *
       * @param fret1 Key to use for fret 1.
       * @param fret2 Key to use for fret 2.
       * @param fret3 Key to use for fret 3.
       * @param fret4 Key to use for fret 4.
       * @param fret5 Key to use for fret 5.
       * @param pick Key to use to pick.
       * @param quit Key to use to quit the game.
       */

      Input (Key fret1, Key fret2, Key fret3, Key fret4, Key fret5, Key pick, Key quit);

      /** Query if a fret key is pressed down.
       *
       * @param fret The fret to query. Must be in range [1, 5].
       * @return If the given fret key is pressed down.
       */

      bool
      fret_pressed (int fret);

      /** Query if the pick key is pressed down.
       *
       * @return If the pick key is pressed down.
       */

      bool
      pick_pressed ();

      /** Query if the quit key is pressed down.
       *
       * @return If the quit key is pressed down.
       */

      bool
      quit_pressed ();

      void key_event (Key key, KeyAction action);

    private:

      struct
      {
        Key keys[7];
        bool pressed[7];
      } state;
  };
}

#endif

