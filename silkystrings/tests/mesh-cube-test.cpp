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
#include <GLExtensionProxy.h>
#include <VertexDataBufferManager.h>
#include <Mesh.h>
#include <VertexFormat.h>
#include <GL.h>

#include <cassert>
#include <vector>
#include <cmath>

#include <boost/shared_ptr.hpp>

/*
 * Classic test for Mesh: Spinning cube :D
 *
 * but for your pleasure, we enhanced it to :drumroll: :lol:
 * a spinning AND a "pitching" cube.
 *
 * The one spinning is drawn using non-indexed vertices and the other one is
 * drawn by (uselessly) specifying the same vertices through indices.
 */

int main()
{
  using namespace SilkyStrings;
  boost::shared_ptr<WM> wm (new WM (640, 480, false));
  boost::shared_ptr<GLExtensionProxy> proxy (new GLExtensionProxy (wm));
  boost::shared_ptr<VertexDataBufferManager> mgr (new VertexDataBufferManager (proxy));
  Mesh *p_mesh;
  VertexFormat format;

  try
  {
    format.insert (VertexFormat::DataMemberSpec (VertexFormat::DATA_ROLE_POS,
          VertexFormat::DATA_TYPE_FLOAT, 3));
    format.insert (VertexFormat::DataMemberSpec (VertexFormat::DATA_ROLE_NORMAL,
          VertexFormat::DATA_TYPE_FLOAT, 3));
  }
  catch (...)
  {
    /* shouldn't have thrown */
    assert (false);
  }
  
  try
  {
    p_mesh = new Mesh (format, mgr, proxy);
  }
  catch (...)
  {
    assert (false);
  }

  const GLfloat cube_vertices[] =
  {
    /* left face */
    -1, 1, -1,
    -1, -1, -1,
    -1, -1, 1,
    -1, 1, 1,

    /* right face */
    1, 1, 1,
    1, -1, 1,
    1, -1, -1,
    1, 1, -1,

    /* front face */
    -1, 1, 1,
    -1, -1, 1,
    1, -1, 1,
    1, 1, 1,

    /* back face */
    1, 1, -1,
    1, -1, -1,
    -1, -1, -1,
    -1, 1, -1,

    /* top face */
    -1, 1, -1,
    -1, 1, 1,
    1, 1, 1,
    1, 1, -1,

    /* bottom face */
    1, -1, 1,
    -1, -1, 1,
    -1, -1, -1,
    1, -1, -1
  };

  const GLfloat cube_normals[] =
  {
    /* left face */
    -1, 0, 0,

    /* right face */
    1, 0, 0,

    /* front face */
    0, 0, 1,

    /* back face */
    0, 0, -1,

    /* top face */
    0, 1, 0,

    /* bottom face */
    0, -1, 0
  };

  for (int face = 0; face < 6; face++)
  {
    for (int vertex = 0; vertex < 4; vertex++)
    {
      std::vector<VertexDataElement> mesh_vertex;

      mesh_vertex.insert (mesh_vertex.end (), cube_vertices + face * 4 * 3 + vertex * 3, cube_vertices + face * 4 * 3 + (vertex + 1) * 3);
      mesh_vertex.insert (mesh_vertex.end (), cube_normals + face * 3, cube_normals + (face + 1) * 3);

      VertexFormat::UINT index = p_mesh->add_vertex (mesh_vertex);

      assert (index == VertexFormat::UINT (face * 4 + vertex));

      assert (p_mesh->add_index (VertexFormat::USHORT (index)) == index);
    }
  }

  while (!wm->update_beginning_of_frame ())
  {
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT0);
    glEnable (GL_LIGHT1);
    glEnable (GL_DEPTH_TEST);

    GLfloat light0_diffuse[] = {1, 1, 1, 1};
    GLfloat light1_diffuse[] = {0.3, 0.3, 0.35, 1};

    GLfloat light0_pos[] = {0, 0, 1, 0};
    GLfloat light1_pos[] = {0, 0, 0.3, 1};

    for (int i = 0; i < 2; i++)
      light1_diffuse[i] += 0.2f * std::sin (float (wm->get_clock () + i) * 4.3f);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();

    gluPerspective (45, 640.0/480.0, 0.1, 100);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();

    glLightfv (GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv (GL_LIGHT1, GL_DIFFUSE, light1_diffuse);

    glLightfv (GL_LIGHT0, GL_POSITION, light0_pos);

    glTranslatef (0, 0, -8);

    glPushMatrix ();
    glRotatef (wm->get_clock () * 89, 1, 0.1, 0.12);
    glLightfv (GL_LIGHT1, GL_POSITION, light1_pos);
    glPopMatrix ();

    glRotatef (wm->get_clock () * 43, 0.2, 1, 0.2);

    glPushMatrix ();
    glTranslatef (-2, 0, 0);
    glRotatef (wm->get_clock () * 131, 0.1, 0.9, 0.13);

    p_mesh->render (Mesh::RENDER_MODE_QUAD_LIST);

    glPopMatrix ();
    glTranslatef (2, 0, 0);
    glRotatef (wm->get_clock () * 119, 0.8, 0.12, 0.13);

    p_mesh->render_indexed (Mesh::RENDER_MODE_QUAD_LIST);

    wm->update_end_of_frame ();
  }

  delete p_mesh;

  return 0;
}

