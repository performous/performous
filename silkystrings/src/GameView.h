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

#ifndef __SS_GAMEVIEW_H_
#define __SS_GAMEVIEW_H_

#include <boost/shared_ptr.hpp>

namespace SilkyStrings
{
  struct WM;

  /**
   * The GameView class provides services for displaying the current game state.
   */

  class GameView
  {
    public:

      /** Constructor.
       *
       * @param visible_secs The timeframe the chords visible at once should span.
       * @param head_threshold The time window in which the player can
       * succesfully start holding a chord down.
       * @param wm A valid WM instance.
       */

      GameView (double visible_secs,
                double head_threshold,
                boost::shared_ptr<WM> wm);

      /** Update the object's state.
       *
       * Call at beginning of each frame, but after calling
       * WM::update_beginning_of_frame()
       *
       * @param t The current game time, counting from the beginning of the
       * song.
       */

      void
      update_beginning_of_frame (double t);

      /**
       * Visual hint about a chord's state.
       */

      enum ChordViewState
      {
        /**
         * The chord is in a passive state.
         */

        CHORD_VIEW_STATE_PASSIVE,

        /**
         * The chord is being held down by the player, but not yet finished.
         */

        CHORD_VIEW_STATE_HELD_DOWN,

        /**
         * The chord has been failed.
         */

        CHORD_VIEW_STATE_FAILED
      };

      /** Draw a beat (quarter note) line.
       *
       * Call before calling draw_chord() for best results.
       *
       * @param time The time in seconds when this beat occurs.
       */
      void draw_beat (double time);

      /** Draw a chord.
       *
       * Call after calling update_beginning_of_frame() for best results.
       *
       * There's no harm in calling this function for chords which are not
       * visible at the moment; however it is not optimal.
       *
       * @param fret The fret number which the chord belongs to. Must be in
       * range [1, 5].
       * @param start_time The time at which the chord starts, counting from the
       * beginning of the song.
       * @param end_time The time at which the chord ends, counting from the
       * beginning of the song.
       * @param state The visible state of the chord.
       */

      void
      draw_chord (unsigned fret,
                  double start_time,
                  double end_time,
                  ChordViewState state);

      /** Draw a fret.
       *
       * Call after calling draw_chord() for best results.
       *
       * @param fret The number of the fret. Must be in range [1, 5].
       * @param held If the user is currently holding the fret down.
       */

      void
      draw_fret (unsigned fret,
                 bool held);

      /** Display a static notification.
       *
       * Display a static notification like a score to the user. The
       * notification can be cleared by displaying an empty notification.
       *
       * @param notification The notification to display.
       */

      void
      set_static_notification (const ::std::string &notification);

      /** Display a pop-up notification.
       *
       * Display a pop-up notification, designed to attract the player's
       * attention. The notification will be displayed fading in and out of
       * vision in the given amount of time.
       *
       * @param notification The notification to display.
       * @param duration How long to display the notification.
       */

      void
      set_popup_notification (const ::std::string &notification,
                                  double duration);

      /** Update the object's state.
       *
       * Call after drawing each frame, but before calling
       * WM::update_end_of_frame() for best results.
       */

      void
      update_end_of_frame ();

    private:

      struct Private;
      boost::shared_ptr<Private> priv;
  };
}

#endif

