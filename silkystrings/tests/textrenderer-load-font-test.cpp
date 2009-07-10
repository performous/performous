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

#include <TextRenderer.h>
#include <WM.h>
#include <Input.h>
#include <Key.h>
#include <GLExtensionProxy.h>
#include <GL.h>
#include <Util.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <iterator>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

int main ()
{
  using namespace SilkyStrings;
  boost::shared_ptr<WM> wm (new WM (640, 480, false));
  boost::shared_ptr<Input> input (new Input (KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_ENTER, KEY_ESC));
  boost::shared_ptr<GLExtensionProxy> proxy (new GLExtensionProxy (wm));
  TextRenderer renderer (SS_FONT, 64, proxy);

  wm->register_client (input);

  std::ifstream ifs ("test-fortunes-omg");
  std::vector<std::string> test_crap = std::vector<std::string> (std::istream_iterator<std::string> (ifs), std::istream_iterator<std::string> ());

  std::vector<std::string>::iterator i = test_crap.begin () - 1;

  float journey = 0.0f;
  float journey_beg = 0.0f;
  float rotation = 0.0f;
  float vel = 0.0f;
  float rot_vel = 0.0f;
  float r, g, b;
  double time_accum;
  unsigned frame_accum;
  double last_time = 0.0f;
  while (!input->quit_pressed () && i != test_crap.end () && !wm->update_beginning_of_frame ())
  {
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (journey <= 0.00001f)
    {
      renderer.discard (*i);
      journey_beg = journey = 15 + std::rand () % 8;
      rotation = -30 + std::rand () % 60;
      vel = 15.0f;
      rot_vel = -30.0f + std::rand () % 60;
      i++;
      last_time = wm->get_clock ();
      r = 0.1f + float (std::rand () % 10) / 10;
      g = 0.1f + float (std::rand () % 10) / 10;
      b = 0.1f + float (std::rand () % 10) / 10;

      time_accum = 0;
      frame_accum = 0;

      std::cout<<"Coming up: "<<*i<<std::endl;
    }

    double dt = std::max (1 / 1000000.0, wm->get_clock () - last_time);
    
    journey -= vel * dt;
    rotation += rot_vel * dt;
    last_time = wm->get_clock ();
    time_accum += dt;
    frame_accum++;

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    gluPerspective (45, 1.333, 1, 100);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
    glTranslatef (0, 0, -7 - journey);
    glRotatef (rotation, 0, 0, 1);

    glColor4f (r, g, b, std::min (1.0f, std::min (journey * 0.15f, (journey_beg - journey) * 0.07f)));

    renderer.render (*i);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho (-1, 1, 0, 1, 0, 1);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
    glTranslatef (0, 0.9, 0);
    glScalef (0.1, 0.1, 0.1);

    std::ostringstream os;
    os << unsigned (frame_accum / time_accum) << " FPS";

    glColor4f (1, 1, 1, 0.6);

    renderer.render (os.str ());
    /* we don't discard the FPS strings because we are going to show them a LOT of times */

    wm->update_end_of_frame ();
  }

  return 0;
}

