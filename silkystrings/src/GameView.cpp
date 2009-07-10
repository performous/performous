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

#include "GameView.h"
#include "WM.h"
#include "GLExtensionProxy.h"
#include "MeshFactory.h"
#include "Mesh.h"
#include "VertexDataBufferManager.h"
#include "GL.h"
#include "Util.h"
#include "TextRenderer.h"

#include <stdexcept>
#include <list>
#include <string>
#include <functional>
#include <cmath>
#include <cassert>
#include <cstdlib>

#include <boost/shared_ptr.hpp>
#include <boost/lambda/lambda.hpp>

using namespace boost::lambda;

namespace SilkyStrings
{
  namespace
  {
    /*
     * RAII GL matrix stack handling :D
     */

    struct
    OrthoMode
    {
      OrthoMode ()
      {
        glMatrixMode (GL_PROJECTION);
        glPushMatrix ();
        glLoadIdentity ();
        glOrtho (-1, 1, -1, 1, -1, 1);
        glMatrixMode (GL_MODELVIEW);
      }

      ~OrthoMode ()
      {
        glMatrixMode (GL_PROJECTION);
        glPopMatrix ();
        glMatrixMode (GL_MODELVIEW);
      }
    };

    struct
    PopupNotification
    {
      std::string text;
      double start, end;

      inline PopupNotification (const std::string &text,
                                double start,
                                double end)
        : text(text), start(start), end(end)
      {
        if (end <= start)
          throw std::logic_error ("end <= start");
      }
    };
  }

  struct GameView::Private
  {
    boost::shared_ptr<WM> wm;
    boost::shared_ptr<GLExtensionProxy> proxy;
    boost::shared_ptr<VertexDataBufferManager> mgr;
    boost::shared_ptr<MeshFactory> factory;
    boost::shared_ptr<TextRenderer> static_renderer;
    boost::shared_ptr<TextRenderer> popup_renderer;

    Mesh head_mesh;
    Mesh tail_mesh;

    double visible;
    double threshold;

    double curr_time;

    std::string static_notification;
    std::list<PopupNotification> popups;

    inline Private (boost::shared_ptr<WM> wm)
      : wm(wm),
        proxy(new GLExtensionProxy (wm)),
        mgr(new VertexDataBufferManager (proxy)),
        factory(new MeshFactory (mgr, proxy)),
        static_renderer(new TextRenderer (SS_FONT, 64, proxy)),
        popup_renderer(new TextRenderer (SS_FONT, 128, proxy)),
        head_mesh(factory->create_head ()),
        tail_mesh(factory->create_tail ())
    {
      visible = 0;
      threshold = 0;
      curr_time = 0;
    }

    void
    draw_static_notification ();

    void
    draw_popup_notifications ();
  };

  GameView::GameView (double visible,
                      double threshold,
                      boost::shared_ptr<WM> wm)
    : priv(new Private (wm))
  {
    if (visible <= 0.0)
      throw std::out_of_range ("Invalid visibility range");

    if (threshold <= 0.0 || threshold > visible)
      throw std::out_of_range ("Invalid accuracy threshold");

    priv->visible = visible;
    priv->threshold = threshold;
  }

  void
  GameView::update_beginning_of_frame (double t)
  {
    glClearColor (1, 1, 1, 1);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    if (priv->proxy->has_generate_mipmap ())
      glHint (GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);

    static GLfloat light[] = {0, 0.5, 1, 0};

    glLightfv (GL_LIGHT0, GL_POSITION, light);
    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT0);

    glEnable (GL_COLOR_MATERIAL);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    gluPerspective (45.0f, 1.333, 1, 15 + std::cos (M_PI / 6) * 15);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();

    glDepthFunc (GL_LEQUAL);
    glCullFace (GL_BACK);

    glEnable (GL_DEPTH_TEST);
    glEnable (GL_CULL_FACE);

    if (!std::getenv ("SS_DISABLE_FOG"))
    {
      GLfloat fog_color[] = {1, 1, 1, 1};
      glHint (GL_FOG_HINT, GL_NICEST);
      glFogi (GL_FOG_MODE, GL_LINEAR);
      glFogfv (GL_FOG_COLOR, fog_color);
      glFogf (GL_FOG_START, 15 + std::cos (M_PI / 6) * 10);
      glFogf (GL_FOG_END, 15 + std::cos (M_PI / 6) * 15);

      glEnable (GL_FOG);
    }

    glPushMatrix ();
    glTranslatef (0, -5, -15);
    glRotatef (30, 1, 0, 0);

    glPushAttrib (GL_ENABLE_BIT);
    glDisable (GL_LIGHTING);

    glColor3f (0.5, 0.5, 0.5);

    glPolygonMode (GL_FRONT, GL_LINE);

    GLfloat rad = (priv->threshold / priv->visible) * 15;
    glBegin (GL_QUADS);
    {
      glVertex3f (-6, -0.251, rad);
      glVertex3f (6, -0.251, rad);
      glVertex3f (6, -0.251, -(rad));
      glVertex3f (-6, -0.251, -(rad));
    }
    glEnd ();

    glPolygonMode (GL_FRONT, GL_FILL);

    glPopMatrix ();
    glPopAttrib ();

    priv->curr_time = t;
  }

  namespace
  {
    GLfloat fret_colors[] =
    {
      1, 0, 0,
      1, 1, 0,
      0, 1, 0,
      0, 1, 1,
      0, 0, 1
    };
  }

  void GameView::draw_beat (double time) {
    if (time < priv->curr_time - priv->visible
        || time > priv->curr_time + priv->visible)
      return;

    glPushMatrix ();
    glPushAttrib (GL_ENABLE_BIT);

    glDisable (GL_LIGHTING);

    glTranslatef (0, -5, -15);
    glRotatef (30, 1, 0, 0);
    glTranslatef (0, 0, -((time - priv->curr_time) * (15 / priv->visible)));
    glColor3f (0.5, 0.5, 0.5);
    GLfloat rad = 0.02;
    glBegin (GL_QUADS);
    {
      glVertex3f (-6, 0, rad);
      glVertex3f (6, 0, rad);
      glVertex3f (6, 0, -rad);
      glVertex3f (-6, 0, -rad);
    }
    glEnd ();

    glPopMatrix ();
    glPopAttrib ();
  }

  void
  GameView::draw_chord (unsigned fret,
                        double start_time,
                        double end_time,
                        ChordViewState state)
  {
    if (fret < 1 || fret > 5)
      throw std::out_of_range ("fret index out of range");

    if (end_time < priv->curr_time - priv->visible
        || start_time + priv->threshold > priv->curr_time + priv->visible)
      return;

    glPushMatrix ();

    double x = (int (fret) - 3) * 2.2;
    double threshold_ratio = priv->threshold / priv->visible;

    glTranslatef (x, -5, -15);
    glRotatef (30, 1, 0, 0);
    glTranslatef (0, 0, -((start_time - priv->curr_time) * (15 / priv->visible)));
    glScalef (15, 15, 15);

    glPushAttrib (GL_ENABLE_BIT);

    if (priv->proxy->has_rescale_normal ())
      glEnable (GL_RESCALE_NORMAL_EXT);
    else
      glEnable (GL_NORMALIZE);

    glPushMatrix ();
    glScalef (threshold_ratio, threshold_ratio, threshold_ratio);

    GLfloat r = fret_colors [(fret - 1) * 3 + 0];
    GLfloat g = fret_colors [(fret - 1) * 3 + 1];
    GLfloat b = fret_colors [(fret - 1) * 3 + 2];

    switch (state)
    {
      case CHORD_VIEW_STATE_PASSIVE:
        r = 0.1 + 0.5 * r;
        g = 0.1 + 0.5 * g;
        b = 0.1 + 0.5 * b;
        break;
      case CHORD_VIEW_STATE_HELD_DOWN:
        break;
      case CHORD_VIEW_STATE_FAILED:
        r = 0.2 + 0.1 * r;
        g = 0.2 + 0.1 * g;
        b = 0.2 + 0.1 * b;
        break;
      default:
        assert (false);
        return;
    }

    glColor3f (r, g, b);

    priv->head_mesh.render_indexed ();

    glPopMatrix ();
    glPopAttrib ();

    glPushAttrib (GL_ENABLE_BIT);
    
    glEnable (GL_NORMALIZE);

    glScalef (threshold_ratio, threshold_ratio, (end_time - start_time) / priv->visible);

    priv->tail_mesh.render_indexed ();

    glPopAttrib ();
    glPopMatrix ();
  }

  void
  GameView::draw_fret (unsigned fret,
                       bool held)
  {
    if (fret < 1 || fret > 5)
      throw std::out_of_range ("fret index out of range");

    if (!held)
      return;

    glPushMatrix ();
    glPushAttrib (GL_ENABLE_BIT);

    glDisable (GL_LIGHTING);

    GLfloat r = 0.5 * fret_colors [(fret - 1) * 3 + 0];
    GLfloat g = 0.5 * fret_colors [(fret - 1) * 3 + 1];
    GLfloat b = 0.5 * fret_colors [(fret - 1) * 3 + 2];

    double x = (int (fret) - 3) * 2.2;

    glTranslatef (x, -5, -15);
    glRotatef (30, 1, 0, 0);

    glColor3f (r, g, b);

    GLfloat rad = 0.05;
    glBegin (GL_QUADS);
    {
      glVertex3f (-(6.0/5.0), -0.2505, rad);
      glVertex3f (6.0/5.0, -0.2505, rad);
      glVertex3f (6.0/5.0, -0.2505, -(rad));
      glVertex3f (-(6.0/5.0), -0.2505, -(rad));
    }
    glEnd ();

    glPopMatrix ();
    glPopAttrib ();
  }

  void
  GameView::set_static_notification (const ::std::string &notification)
  {
    if (notification != priv->static_notification)
      priv->static_renderer->discard (priv->static_notification);

    priv->static_notification = notification;
  }

  void
  GameView::set_popup_notification (const ::std::string &notification,
                                    double duration)
  {
    if (notification.length () == 0)
      return;

    priv->popups.push_back (PopupNotification (notification,
                                               priv->curr_time,
                                               priv->curr_time + duration));
  }

  void
  GameView::update_end_of_frame ()
  {
    priv->draw_static_notification ();
    priv->draw_popup_notifications ();

    GLenum error;
    while ((error = glGetError ()) != GL_NO_ERROR)
    {
      SS_WARN ("GL error %x (%s)", error, gluErrorString (error));
    }
  }

  void
  GameView::Private::draw_static_notification ()
  {
    OrthoMode mode;

    glPushMatrix ();
    glTranslatef (0, 0.915, 0);
    glScalef (0.12, 0.12, 0.12);
    glColor4f (0, 0, 0, 0.9);
    static_renderer->render (static_notification);

    glPopMatrix ();
  }

  void
  GameView::Private::draw_popup_notifications ()
  {
    for (std::list<PopupNotification>::const_iterator i = popups.begin ();
                                                      i != popups.end ();
                                                      ++i)
    {
      if (i->end < curr_time)
        popup_renderer->discard (i->text);
    }

    popups.remove_if ((&_1) ->* (&PopupNotification::end) < curr_time);

    glDepthMask (GL_FALSE);

    for (std::list<PopupNotification>::const_iterator i = popups.begin ();
                                                      i != popups.end ();
                                                      ++i)
    {
      double ratio = std::max(0.0, (curr_time - i->start)  / (i->end - i->start));
      double opacity = std::sin (ratio * M_PI);

      glPushMatrix ();
      glTranslatef (0, 1.5 - 0.3 * ratio, -(10 - ratio * 1.5));

      glColor4f (0, 0, 0, opacity);
      popup_renderer->render (i->text);

      glPopMatrix ();
    }

    glDepthMask (GL_TRUE);
  }
}

