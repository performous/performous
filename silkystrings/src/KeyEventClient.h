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

#ifndef __SS_KEYEVENTCLIENT_H_
#define __SS_KEYEVENTCLIENT_H_

#include "Key.h"

namespace SilkyStrings
{
  /**
   * The KeyEventClient class is an abstract base class for classes which are
   * able to receive key events from WM.
   */

  class KeyEventClient
  {
    public:

      /**
       * Receive a key event.
       */

      virtual void
        key_event (Key key, KeyAction action) = 0;

      /** 
       * Stub virtual destructor.
       */

      inline virtual ~KeyEventClient () {}
  };
}

#endif

