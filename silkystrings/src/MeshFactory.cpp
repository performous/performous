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

#include "MeshFactory.h"
#include "Mesh.h"
#include "VertexFormat.h"

#include <stdexcept>
#include <cmath>

namespace SilkyStrings
{
  namespace
  {
    VertexFormat
    get_format ()
    {
      VertexFormat ret;
      
      ret.insert (VertexFormat::DataMemberSpec (VertexFormat::DATA_ROLE_POS,
                                                VertexFormat::DATA_TYPE_FLOAT,
                                                3,
                                                false));

      ret.insert (VertexFormat::DataMemberSpec (VertexFormat::DATA_ROLE_NORMAL,
                                                VertexFormat::DATA_TYPE_FLOAT,
                                                3,
                                                false));

      return ret;
    }

    VertexFormat::DataType
    enough_indices (size_t count)
    {
      if (count < (1 << 8))
        return VertexFormat::DATA_TYPE_UBYTE;
      else if (count < (1 << 16))
        return VertexFormat::DATA_TYPE_USHORT;
      else
        return VertexFormat::DATA_TYPE_UINT;
    }

    VertexFormat::DataType
    enough_head_indices (unsigned tessel)
    {
      return enough_indices (1 + 3 * tessel);
    }

    VertexFormat::DataType
    enough_tail_indices (unsigned tessel)
    {
      return enough_indices (2 * tessel);
    }

    void
    load_head (Mesh &mesh, unsigned tessel)
    {
      std::vector<VertexDataElement> vertex;
      const float height = 0.25f;

      /* top center vertex (index 0) */
      vertex.push_back (0.0f);
      vertex.push_back (height);
      vertex.push_back (0.0f);

      vertex.push_back (0.0f);
      vertex.push_back (1.0f);
      vertex.push_back (0.0f);

      mesh.add_vertex (vertex);

      vertex.clear ();

      for (unsigned i = 0; i < tessel; i++)
      {
        float t = float (i) / float (tessel) * float (2 * M_PI);

        vertex.push_back (std::cos (t));
        vertex.push_back (height);
        vertex.push_back (-std::sin (t));

        vertex.push_back (std::cos (t));
        vertex.push_back (0.0f);
        vertex.push_back (-std::sin (t));

        mesh.add_vertex (vertex);

        vertex.clear ();

        vertex.push_back (std::cos (t));
        vertex.push_back (-height);
        vertex.push_back (-std::sin (t));

        vertex.push_back (std::cos (t));
        vertex.push_back (0.0f);
        vertex.push_back (-std::sin (t));

        mesh.add_vertex (vertex);

        vertex.clear ();

        vertex.push_back (std::cos (t));
        vertex.push_back (height);
        vertex.push_back (-std::sin (t));

        vertex.push_back (0.0f);
        vertex.push_back (1.0f);
        vertex.push_back (0.0f);

        mesh.add_vertex (vertex);

        vertex.clear ();
      }

      VertexFormat::DataType type = enough_head_indices (tessel);

      for (unsigned i = 0; i < tessel; i++)
      {
        mesh.add_index (VertexDataElement (type, 0));
        mesh.add_index (VertexDataElement (type, 1 + (i * 3) + 2));
        mesh.add_index (VertexDataElement (type, 1 + ((i + 1) % tessel) * 3 + 2));

        mesh.add_index (VertexDataElement (type, 1 + (i * 3)));
        mesh.add_index (VertexDataElement (type, 1 + (i * 3) + 1));
        mesh.add_index (VertexDataElement (type, 1 + ((i + 1) % tessel) * 3 + 1));

        mesh.add_index (VertexDataElement (type, 1 + ((i + 1) % tessel) * 3));
        mesh.add_index (VertexDataElement (type, 1 + (i * 3)));
        mesh.add_index (VertexDataElement (type, 1 + ((i + 1) % tessel) * 3 + 1));
      }
    }

    void
    normalize2 (float &x, float &y)
    {
      float dot = x*x + y*y;
      float len_rcp = 1.0f / std::sqrt (dot);

      x *= len_rcp;
      y *= len_rcp;
    }

    void
    load_tail (Mesh &mesh, unsigned tessel)
    {
      const float width = 0.75f;
      const float height = 0.35f;
      const float floor = -0.1f;

      std::vector<VertexDataElement> vertex;

      for (unsigned i = 0; i < tessel; i++)
      {
        float t = float (i) / float (tessel) * float(M_PI);
        float nx = height * std::cos (t);
        float ny = width * std::sin (t);

        normalize2 (nx, ny);

        vertex.push_back (width * std::cos (t));
        vertex.push_back (floor + height * std::sin (t));
        vertex.push_back (0.0f);

        vertex.push_back (nx);
        vertex.push_back (ny);
        vertex.push_back (0.0f);

        mesh.add_vertex (vertex);

        vertex.clear ();

        vertex.push_back (width * std::cos (t));
        vertex.push_back (floor + height * std::sin (t));
        vertex.push_back (-1.0f);

        vertex.push_back (nx);
        vertex.push_back (ny);
        vertex.push_back (0.0f);

        mesh.add_vertex (vertex);

        vertex.clear ();
      }

      VertexFormat::DataType type = enough_tail_indices (tessel);
      for (unsigned i = 0; i < tessel - 1; i++)
      {
        mesh.add_index (VertexDataElement (type, i * 2));
        mesh.add_index (VertexDataElement (type, i * 2 + 1));
        mesh.add_index (VertexDataElement (type, ((i + 1) % tessel) * 2 + 1));

        mesh.add_index (VertexDataElement (type, ((i + 1) % tessel) * 2 + 1));
        mesh.add_index (VertexDataElement (type, ((i + 1) % tessel) * 2));
        mesh.add_index (VertexDataElement (type, i * 2));
      }
    }
  }

  MeshFactory::MeshFactory (boost::weak_ptr<VertexDataBufferManager> mgr,
                            boost::shared_ptr<GLExtensionProxy> proxy,
                            unsigned head_tessel,
                            unsigned tail_tessel)
    : head (get_format (), mgr, proxy, enough_head_indices (head_tessel)),
      tail (get_format (), mgr, proxy, enough_tail_indices (tail_tessel))
  {
    if (head_tessel < 3 || tail_tessel < 2)
      throw std::logic_error ("impossibly low tessellation requested");

    load_head (head, head_tessel);
    load_tail (tail, tail_tessel);
  }

  Mesh
  MeshFactory::create_head ()
  {
    return head;
  }

  Mesh
  MeshFactory::create_tail ()
  {
    return tail;
  }
}

