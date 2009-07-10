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
#include <Input.h>
#include <Key.h>

#include <boost/shared_ptr.hpp>

int main()
{
  using namespace SilkyStrings;

  WM wm(640, 480, false);
  boost::shared_ptr<Input> input(new Input(KEY_O, KEY_A, KEY_I, KEY_E, KEY_D, KEY_SPACE, KEY_ESC));

  wm.register_client (input);

  while (!wm.update_beginning_of_frame ())
    wm.update_beginning_of_frame ();

  return 0;
}

