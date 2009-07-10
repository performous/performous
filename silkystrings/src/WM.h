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

#ifndef __SS_WM_H_
#define __SS_WM_H_

#include <string>
#include <set>
#include <boost/weak_ptr.hpp>

namespace SilkyStrings
{
  struct KeyEventClient;

  /**
   * The WM class provides window management.
   */

  class WM
  {
    public:

      /** Constructor.
       *
       * Constructs a new instance, opening a new window. Note that because of
       * GLFW's restrictions, only a single WM instance and hence a single
       * window can be opened at a time. The constructor will throw on attempts
       * to initialize multiple instances simultaneously. Will also throw on
       * being unable to open the window from various reasons.
       *
       * @param width The width of the window to open.
       * @param height The height of the window to open.
       * @param fullscreen If the window should be opened fullscreen.
       */

      WM (int width, int height, bool fullscreen);

      /** Destructor.
       *
       * Closes the opened window, stops sending events to any
       * registered Input client and allows subsequent instanciations to
       * succeed.
       */

      ~WM ();

      /** Register a keyevent client.
       *
       * Registers the given KeyEventClient instance as a client for this WM
       * object. The WM object will provide keyboard events to it. Will throw
       * if the given client is already registered.
       *
       * @param client The instance to register as a client.
       */

      void
      register_client (boost::weak_ptr<KeyEventClient> client);

      /** Unregister a keyevent client.
       *
       * Unregisters the given keyevent client from the instance. Will throw if
       * it isn't currently a client.
       *
       * @param client The client to unregister.
       */

      void
      unregister_client (boost::weak_ptr<KeyEventClient> client);

      /** Updates the WM's state.
       *
       * Should be called once in the beginning of every mainloop iteration.
       *
       * @return If the window has been closed and the application should exit.
       */

      bool
      update_beginning_of_frame ();

      /** Updates the WM's state.
       *
       * Should be called once in the end of every mainloop iteration.
       */

      void
      update_end_of_frame ();

      /** Get elapsed time since initialization.
       *
       * Get the amount of time elapsed in seconds since this WM instance was
       * initialized.
       *
       * @return The elapsed time.
       */

      double
      get_clock ();

      /** Take a nap.
       *
       * Sleep for the given amount of time (in seconds). Only affects the
       * caller thread. Other threads of the process, if any, are unaffected.
       *
       * @param duration The amount of time to sleep for.
       */

      void
      sleep (double duration);

      /** Check for GL/WGL/GLX extension availability.
       *
       * Checks if the given OpenGL/WGL/GLX extension is supported when
       * rendering to this window. get_gl_extension_proc() is guaranteed to
       * succeed with functions pertaining to an extension if this function
       * reports it as supported.
       *
       * @param extension_name The extension to query for.
       *
       * @return If the extension is supported.
       */

      bool
      has_gl_extension (const ::std::string &extension_name);

      /** Get GL/WGL/GLX extension procedure address.
       *
       * Returns a pointer to the given OpenGL/WGL/GLX extension procedure.
       * Throws if the related extension isn't supported. has_gl_extension
       * should be called first with the name of the extension to check for
       * support.
       *
       * @return The address of the requested procedure.
       */

      void *
      get_gl_extension_proc (const ::std::string &extension_proc_name);

    private:

      static void key_cb (int, int);
      static int close_cb ();

      std::set<boost::weak_ptr<KeyEventClient> > keyev_clients;
      bool closed;
      static WM *instance;
  };
}

#endif

