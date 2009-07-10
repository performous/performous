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

#include "WM.h"
#include "Input.h"
#include "Key.h"
#include "Util.h"
#include "KeyEventClient.h"

#include <stdexcept>
#include <string>
#include <set>
#include <cassert>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <GL/glfw.h>

namespace SilkyStrings
{
  WM *WM::instance;

  WM::WM (int width, int height, bool fullscreen)
  {
    if (instance)
      throw std::runtime_error ("Only one WM instance at a time is supported");

    int glfw_mode = fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW;

    if (glfwInit () != GL_TRUE)
      throw std::runtime_error ("Failed to initialize GLFW");

    if (glfwOpenWindow (width, height, 8, 8, 8, 0, 24, 0, glfw_mode) != GL_TRUE)
      throw std::runtime_error ("Failed to open GLFW window");

    glfwSetWindowTitle ("SilkyStrings");

    glfwEnable (GLFW_STICKY_KEYS);
    glfwDisable (GLFW_AUTO_POLL_EVENTS);
    glfwSetKeyCallback (key_cb);

    glfwSetWindowCloseCallback (close_cb);

    glClear (GL_COLOR_BUFFER_BIT);

    closed = false;

    instance = this;
  }

  WM::~WM ()
  {
    glfwTerminate ();

    instance = NULL;
  }

  void
  WM::register_client (boost::weak_ptr<KeyEventClient> client)
  {
    if (keyev_clients.find (client) != keyev_clients.end ())
      throw std::logic_error ("Tried to insert the same KeyEventClient twice");

    keyev_clients.insert (client);
  }

  void
  WM::unregister_client (boost::weak_ptr<KeyEventClient> client)
  {
    if (keyev_clients.find (client) == keyev_clients.end ())
      throw std::logic_error ("Tried to unregister a unknown KeyEventClient");

    keyev_clients.erase (client);
  }

  bool
  WM::update_beginning_of_frame ()
  {
    glfwPollEvents ();

    return closed;
  }

  void
  WM::update_end_of_frame ()
  {
    glfwSwapBuffers ();
  }

  void
  WM::key_cb (int key, int event)
  {
    Key internal_key;
    KeyAction internal_keyaction;

    assert (instance != NULL);

    if (key >= 'A' && key <= 'Z')
    {
      internal_key = Key (KEY_A + (key - 'A'));
    }
    else if (key >= '0' && key <= '9')
    {
      internal_key = Key (KEY_0 + (key - '0'));
    }
    else if (key == GLFW_KEY_SPACE)
    {
      internal_key = KEY_SPACE;
    }
    else if (key >= GLFW_KEY_ESC && key <= GLFW_KEY_F12)
    {
      internal_key = Key (KEY_ESC + (key - GLFW_KEY_ESC));
    }
    else if (key >= GLFW_KEY_UP && key < GLFW_KEY_LAST)
    {
      internal_key = Key (KEY_UP + (key - GLFW_KEY_UP));
    }
    else
    {
      SS_WARN ("unhandled GLFW keycode: %d", key);
      return;
    }

    switch (event)
    {
      case GLFW_PRESS:
        internal_keyaction = KEY_ACTION_PRESS;
        break;
      case GLFW_RELEASE:
        internal_keyaction = KEY_ACTION_RELEASE;
        break;
      default:
        SS_WARN ("unhandled GLFW keyevent: %d", key);
        return;
    }

    for (std::set<boost::weak_ptr<KeyEventClient> >::iterator i
             = instance->keyev_clients.begin ();
          i != instance->keyev_clients.end ();
          i++)
    {
      boost::shared_ptr<KeyEventClient> client = i->lock ();

      if (client)
        client->key_event (internal_key, internal_keyaction);
    }
  }

  int
  WM::close_cb ()
  {
    instance->closed = true;
    return GL_TRUE;
  }

  double
  WM::get_clock ()
  {
    return glfwGetTime ();
  }

  void
  WM::sleep (double dur)
  {
    return glfwSleep (dur);
  }

  bool
  WM::has_gl_extension (const ::std::string &ext)
  {
    return glfwExtensionSupported (ext.c_str ()) == GL_TRUE;
  }

  void *
  WM::get_gl_extension_proc (const ::std::string &proc)
  {
    void *procaddr = glfwGetProcAddress (proc.c_str ());

    if (procaddr == NULL)
      throw std::runtime_error ("No support for the requested OpenGL/WGL/GLX extension");

    return procaddr;
  }
}

