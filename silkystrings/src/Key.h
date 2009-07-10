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

#ifndef __SS_KEY_H_
#define __SS_KEY_H_

namespace SilkyStrings
{
  /**
   * The Key enum contains the symbolic values used to represent physical keys.
   */

  enum Key
  {
    KEY_SPACE = 0, /**< The Spacebar key.*/
    KEY_ESC, /**< The Escape key. */

    KEY_F1, /**< The F1 key. */
    KEY_F2, /**< The F2 key. */
    KEY_F3, /**< The F3 key. */
    KEY_F4, /**< The F4 key. */
    KEY_F5, /**< The F5 key. */
    KEY_F6, /**< The F6 key. */
    KEY_F7, /**< The F7 key. */
    KEY_F8, /**< The F8 key. */
    KEY_F9, /**< The F9 key. */
    KEY_F10, /**< The F10 key. */
    KEY_F11, /**< The F11 key. */
    KEY_F12, /**< The F12 key. */

    KEY_UP, /**< The up arrow key. */
    KEY_DOWN, /**< The down arrow key. */
    KEY_LEFT, /**< The left arrow key. */
    KEY_RIGHT, /**< The right arrow key. */

    KEY_LSHIFT, /**< The left Shift key. */
    KEY_RSHIFT, /**< The right Shift key. */
    KEY_LCTRL, /**< The left Control key. */
    KEY_RCTRL, /**< The right Control key. */
    KEY_LALT, /**< The left Alt key. */
    KEY_RALT, /**< The right Alt key. */

    KEY_TAB, /**< The Tab key. */
    KEY_ENTER, /**< The Enter key. */
    KEY_BACKSPACE, /**< The Backspace key. */

    KEY_INSERT, /**< The Insert key. */
    KEY_DEL, /**< The Del key. */
    KEY_PAGEUP, /**< The Page Up key. */
    KEY_PAGEDOWN, /**< The Page Down key. */
    KEY_HOME, /**< The Home key. */
    KEY_END, /**< The End key. */

    KEY_KP_0, /**< The keypad 0 key. */
    KEY_KP_1, /**< The keypad 1 key. */
    KEY_KP_2, /**< The keypad 2 key. */
    KEY_KP_3, /**< The keypad 3 key. */
    KEY_KP_4, /**< The keypad 4 key. */
    KEY_KP_5, /**< The keypad 5 key. */
    KEY_KP_6, /**< The keypad 6 key. */
    KEY_KP_7, /**< The keypad 7 key. */
    KEY_KP_8, /**< The keypad 8 key. */
    KEY_KP_9, /**< The keypad 9 key. */
    KEY_KP_DIVIDE, /**< The keypad Divide key. */
    KEY_KP_MULTIPLY, /**< The keypad Multiply key. */
    KEY_KP_SUBTRACT, /**< The keypad Subtract key. */
    KEY_KP_ADD, /**< The keypad Add key. */
    KEY_KP_DECIMAL, /**< The keypad Decimal key. */
    KEY_KP_EQUAL, /**< The keypad Equal key. */
    KEY_KP_ENTER, /**< The keypad Enter key. */

    KEY_A, /**< The A key. */
    KEY_B, /**< The B key. */
    KEY_C, /**< The C key. */
    KEY_D, /**< The D key. */
    KEY_E, /**< The E key. */
    KEY_F, /**< The F key. */
    KEY_G, /**< The G key. */
    KEY_H, /**< The H key. */
    KEY_I, /**< The I key. */
    KEY_J, /**< The J key. */
    KEY_K, /**< The K key. */
    KEY_L, /**< The L key. */
    KEY_M, /**< The M key. */
    KEY_N, /**< The N key. */
    KEY_O, /**< The O key. */
    KEY_P, /**< The P key. */
    KEY_Q, /**< The Q key. */
    KEY_R, /**< The R key. */
    KEY_S, /**< The S key. */
    KEY_T, /**< The T key. */
    KEY_U, /**< The U key. */
    KEY_V, /**< The V key. */
    KEY_W, /**< The W key. */
    KEY_X, /**< The X key. */
    KEY_Y, /**< The Y key. */
    KEY_Z, /**< The Z key. */

    KEY_0, /**< The number row 0 key. */
    KEY_1, /**< The number row 1 key. */
    KEY_2, /**< The number row 2 key. */
    KEY_3, /**< The number row 3 key. */
    KEY_4, /**< The number row 4 key. */
    KEY_5, /**< The number row 5 key. */
    KEY_6, /**< The number row 6 key. */
    KEY_7, /**< The number row 7 key. */
    KEY_8, /**< The number row 8 key. */
    KEY_9, /**< The number row 9 key. */

    KEY_NUMBER_OF /**< The number of keys. Not a valid key symbol. */
  };

  /**
   * The KeyAction enum contains symbolic values to represent actions taken on
   * physical keys.
   */

  enum KeyAction
  {
    KEY_ACTION_PRESS = 0, /**< The key was pressed down */
    KEY_ACTION_RELEASE /**< The key was released */
  };

  /**
   * @todo This array shouldn't really be in a header. But it shouldn't be
   * extern in a implementation file either, since we don't want to link
   * Launcher against silkystrings, but still want to use these strings there.
   */

  namespace
  {
    const char *key_strings[KEY_NUMBER_OF] =
    {
      "SPACE",
      "ESC",

      "F1",
      "F2",
      "F3",
      "F4",
      "F5",
      "F6",
      "F7",
      "F8",
      "F9",
      "F10",
      "F11",
      "F12",

      "UP",
      "DOWN",
      "LEFT",
      "RIGHT",

      "LSHIFT",
      "RSHIFT",
      "LCTRL",
      "RCTRL",
      "LALT",
      "RALT",

      "TAB",
      "ENTER",
      "BACKSPACE",

      "INSERT",
      "DEL",
      "PAGEUP",
      "PAGEDOWN",
      "HOME",
      "END",

      "KP_0",
      "KP_1",
      "KP_2",
      "KP_3",
      "KP_4",
      "KP_5",
      "KP_6",
      "KP_7",
      "KP_8",
      "KP_9",
      "KP_DIVIDE",
      "KP_MULTIPLY",
      "KP_SUBTRACT",
      "KP_ADD",
      "KP_DECIMAL",
      "KP_EQUAL",
      "KP_ENTER",

      "A",
      "B",
      "C",
      "D",
      "E",
      "F",
      "G",
      "H",
      "I",
      "J",
      "K",
      "L",
      "M",
      "N",
      "O",
      "P",
      "Q",
      "R",
      "S",
      "T",
      "U",
      "V",
      "W",
      "X",
      "Y",
      "Z",

      "0",
      "1",
      "2",
      "3",
      "4",
      "5",
      "6",
      "7",
      "8",
      "9",
    };
  }
}

#endif

