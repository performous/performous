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

#include "Input.h"
#include "Key.h"

#include <algorithm>
#include <stdexcept>

namespace SilkyStrings
{
  enum KeyStateIndex
  {
    KEY_STATE_INDEX_FRET1 = 0,
    KEY_STATE_INDEX_FRET2,
    KEY_STATE_INDEX_FRET3,
    KEY_STATE_INDEX_FRET4,
    KEY_STATE_INDEX_FRET5,
    KEY_STATE_INDEX_PICK,
    KEY_STATE_INDEX_QUIT
  };

  Input::Input (Key fret1, Key fret2, Key fret3, Key fret4, Key fret5, Key pick, Key quit)
  {
    state.keys[KEY_STATE_INDEX_FRET1] = fret1;
    state.keys[KEY_STATE_INDEX_FRET2] = fret2;
    state.keys[KEY_STATE_INDEX_FRET3] = fret3;
    state.keys[KEY_STATE_INDEX_FRET4] = fret4;
    state.keys[KEY_STATE_INDEX_FRET5] = fret5;
    state.keys[KEY_STATE_INDEX_PICK] = pick;
    state.keys[KEY_STATE_INDEX_QUIT] = quit;

    std::fill (state.pressed, state.pressed + 7, false);
  }

  bool
  Input::fret_pressed (int fret)
  {
    if (fret-1 < KEY_STATE_INDEX_FRET1 || fret-1 > KEY_STATE_INDEX_FRET5)
      throw std::logic_error (": Only fret values in range [1, 5] allowed");

    return state.pressed[fret-1];
  }

  bool
  Input::pick_pressed ()
  {
    return state.pressed[KEY_STATE_INDEX_PICK];
  }

  bool
  Input::quit_pressed ()
  {
    return state.pressed[KEY_STATE_INDEX_QUIT];
  }

  void
  Input::key_event (Key key, KeyAction action)
  {
    for (int i = 0; i < 7; i++)
      if (state.keys[i] == key)
        state.pressed[i] = (action == KEY_ACTION_PRESS) ? true : false;
  }
}

