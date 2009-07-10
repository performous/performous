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
#include <VertexDataBufferManager.h>
#include <SoftwareVertexDataBuffer.h>
#include <SoftwareElementDataBuffer.h>
#include <VertexFormat.h>
#include <GLExtensionProxy.h>

#include <string>
#include <algorithm>

#include <iostream>

int main()
{
  using namespace SilkyStrings;

  boost::shared_ptr<WM> wm (new WM (640, 480, false));
  boost::shared_ptr<GLExtensionProxy> proxy(new GLExtensionProxy (wm));
  VertexDataBufferManager mgr(proxy);

  try
  {
    boost::shared_ptr<VertexDataBuffer> vbuf = mgr.create_vertex_data_buffer (size_t(69));
    boost::shared_ptr<ElementDataBuffer> ebuf = mgr.create_element_data_buffer (VertexFormat::DATA_TYPE_UINT);

    assert (vbuf != NULL);
    assert (ebuf != NULL);
  }
  catch (...)
  {
    /* shouldn't have thrown */
    assert (false);
  }

  SoftwareVertexDataBuffer *swvbuf = new SoftwareVertexDataBuffer (proxy, sizeof (char));

  assert (swvbuf != NULL);

  std::string test_str ("ayababtu");

  try
  {
    for (unsigned i = 0; i < test_str.length (); i++)
    {
      swvbuf->write (-1, &test_str[i]);
    }
  }
  catch (...)
  {
    /* shouldn't have thrown */
    assert (false);
  }

  char rbbuf[16] = {0};

  try
  {
    swvbuf->read (0, test_str.length (), (void *) &rbbuf[0]);
  }
  catch (...)
  {
    /* shouldn't have thrown */
    assert (false);
  }

  try
  {
    delete swvbuf;
  }
  catch (...)
  {
    /* shouldn't have thrown */
    assert (false);
  }

  assert (test_str == rbbuf);

  try
  {
    SoftwareElementDataBuffer *swebuf = new SoftwareElementDataBuffer (proxy, VertexFormat::DATA_TYPE_UINT);
    VertexFormat::UINT indices[] = {1, 3, 5, 7, 11, 13, 17, 19};

    try
    {
      for (unsigned i = 0; i < sizeof (indices) / sizeof (VertexFormat::UINT); i++)
      {
        swebuf->write (-1, indices[i]);
      }
    }
    catch (...)
    {
      /* shouldn't have thrown */
      assert (false);
    }

    VertexFormat::UINT rb_indices[6];

    try
    {
      swebuf->read (2, 6, (void *) &rb_indices);
    }
    catch (...)
    {
      /* shouldn't have thrown */
      assert (false);
    }

    if (!std::equal (rb_indices, rb_indices + 6, indices + 2))
      assert (false);
  }
  catch (...)
  {
    assert (false);
  }

  return 0;
}

