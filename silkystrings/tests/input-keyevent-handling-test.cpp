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

#include <Input.h>
#include <Key.h>

#include <cassert>

int main()
{
  using namespace SilkyStrings;

  Input input (KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G);

  /*
   * test that nothing is pressed in the beginning
   */

  for (int i = 1; i <= 5; i++)
    assert (!input.fret_pressed (i));

  assert (!input.pick_pressed ());
  assert (!input.quit_pressed ());

  /*
   * test that reaction to a keypress is correct
   */

  input.key_event (KEY_A, KEY_ACTION_PRESS);
  assert (input.fret_pressed (1));

  /*
   * test that actions don't interfere
   */
  
  input.key_event (KEY_B, KEY_ACTION_PRESS);
  assert (input.fret_pressed (1));
  assert (input.fret_pressed (2));

  /*
   * test that releasing works correctly
   */

  input.key_event (KEY_B, KEY_ACTION_RELEASE);
  assert (input.fret_pressed (1));
  assert (!input.fret_pressed (2));

  input.key_event (KEY_A, KEY_ACTION_RELEASE);
  assert (!input.fret_pressed (1));
  assert (!input.fret_pressed (2));

  return 0;
}

