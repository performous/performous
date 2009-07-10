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

#include <KeyEventClient.h>
#include <Key.h>
#include <WM.h>
#include <TextRenderer.h>
#include <GLExtensionProxy.h>
#include <GL.h>
#include <Util.h>

#include <string>
#include <deque>
#include <cmath>

#include <boost/shared_ptr.hpp>
#include <stdio.h>

using namespace SilkyStrings;

class TestClient : public KeyEventClient
{
  public:

    TestClient ()
      : wm (new WM (640, 480, false)),
        proxy (new GLExtensionProxy (wm)),
        renderer (new TextRenderer (SS_FONT, 128, proxy))
    {
    }

    void
    key_event (Key key, KeyAction action)
    {
      if (action == KEY_ACTION_PRESS)
        presses.push_back (key_strings [key]);
    }

    int
    exec ()
    {
      float last_time = wm->get_clock ();
      while (!wm->update_beginning_of_frame ())
      {
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        gluPerspective (45.0f, 1.333f, 1, 100);

        glMatrixMode (GL_MODELVIEW);
        glLoadIdentity ();
        glTranslatef (0, 0, -5);

        float dt = wm->get_clock () - last_time;
        last_time = wm->get_clock ();

        for (std::deque<KeyPress>::iterator i = presses.begin ();
              i != presses.end ();)
        {
          i->age += dt;

          if (i->age >= 0.3f)
          {
            i = presses.erase (i);
          }
          else
          {
            glColor4f (1, 1, 1, 1.0f - std::fabs (1.0f - 10.0f * i->age));
            renderer->render (i->str);
            i++;
          }
        }

        wm->update_end_of_frame ();
      }

      return 0;
    }

    /* uhh... bad design */
    void
    bad_design (boost::shared_ptr<TestClient> give_me_me)
    {
      wm->register_client (give_me_me);
    }

  private:

    boost::shared_ptr<WM> wm;
    boost::shared_ptr<GLExtensionProxy> proxy;
    boost::shared_ptr<TextRenderer> renderer;

    struct KeyPress
    {
      KeyPress (const char *str)
        : str(str), age(0.0f)
      {
      }

      std::string str;
      float age;
    };

    std::deque<KeyPress> presses;
};

int main ()
{
  boost::shared_ptr<TestClient> client (new TestClient);

  client->bad_design (client);

  return client->exec ();
}

