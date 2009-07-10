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

#include "Mesh.h"
#include "VertexFormat.h"
#include "VertexDataBufferManager.h"
#include "VertexDataBuffer.h"
#include "ElementDataBuffer.h"
#include "GLExtensionProxy.h"
#include "GL.h"
#include "Util.h"

#include <cassert>

#include <vector>
#include <numeric>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

#include <boost/shared_ptr.hpp>

namespace SilkyStrings
{
  Mesh::Private *Mesh::bound_vertices = NULL;
  Mesh::Private *Mesh::bound_indices = NULL;

  struct Mesh::Private
  {
    boost::shared_ptr<GLExtensionProxy> proxy;

    boost::shared_ptr<VertexDataBuffer> static_buf;
    size_t static_size;

    boost::shared_ptr<VertexDataBuffer> dynamic_buf;
    size_t dynamic_size;

    VertexFormat::DataType index_type;
    boost::shared_ptr<ElementDataBuffer> indices;

    VertexFormat vf;

    VertexFormat::UINT index;
    unsigned serial;

    inline Private (boost::shared_ptr<GLExtensionProxy> proxy,
                    const VertexFormat &vf)
      : proxy(proxy), vf(vf), index(0), serial(0)
    {
    }
  };

  namespace
  {
    bool
    vfspec_role_is_pos (const VertexFormat::DataMemberSpec &spec)
    {
      return spec.role == VertexFormat::DATA_ROLE_POS;
    }

    bool
    vfspec_suggests_dynamic (const VertexFormat::DataMemberSpec &spec)
    {
      return spec.dynamic;
    }

    bool
    vfspec_suggests_static (const VertexFormat::DataMemberSpec &spec)
    {
      return !spec.dynamic;
    }

    size_t
    sum_sizes (size_t a, const VertexFormat::DataMemberSpec &b)
    {
      return a + VertexFormat::size_of_type (b.type) * b.count;
    }
  }

  Mesh::Mesh (const VertexFormat &format,
              boost::weak_ptr<VertexDataBufferManager> weak_mgr,
              boost::shared_ptr<GLExtensionProxy> proxy,
              VertexFormat::DataType index_type,
              bool dynamic_indices)
    : priv (new Private (proxy, format))
  {
    if (std::find_if (format.begin (), format.end (), vfspec_role_is_pos)
        == format.end())
      throw std::logic_error ("incomplete vertex format passed");

    std::vector<VertexFormat::DataMemberSpec> static_format;
    std::vector<VertexFormat::DataMemberSpec> dynamic_format (format.begin (),
        format.end ());

    std::remove_copy_if (dynamic_format.begin (), dynamic_format.end (),
        std::back_inserter (static_format), vfspec_suggests_dynamic);

    dynamic_format.erase (std::remove_if (dynamic_format.begin(),
          dynamic_format.end(), vfspec_suggests_static), dynamic_format.end ());

    priv->static_size = std::accumulate (static_format.begin (),
        static_format.end (), size_t (0), sum_sizes);

    priv->dynamic_size = std::accumulate (dynamic_format.begin (),
        dynamic_format.end (), size_t (0), sum_sizes);

    boost::shared_ptr<VertexDataBufferManager> mgr = weak_mgr.lock ();

    if (!mgr)
      throw std::logic_error ("Invalid manager point");

    if (priv->static_size)
      priv->static_buf = mgr->create_vertex_data_buffer (priv->static_size, false);

    if (priv->dynamic_size)
      priv->dynamic_buf = mgr->create_vertex_data_buffer (priv->dynamic_size, true);

    priv->index_type = index_type;
    priv->indices = mgr->create_element_data_buffer (index_type,
        dynamic_indices);
  }

  VertexFormat::UINT
  Mesh::add_vertex (const std::vector<VertexDataElement> &vertex)
  {
    std::vector<char> static_part, dynamic_part;
    std::vector<VertexDataElement>::const_iterator vi = vertex.begin ();

    for (VertexFormat::const_iterator i = priv->vf.begin ();
        i != priv->vf.end(); i++)
    {
      const VertexFormat::DataMemberSpec &spec = *i;

      std::vector<char> &part =
        spec.dynamic ? dynamic_part : static_part;

      for (int i = 0; i < spec.count; i++)
      {
        union
        {
          VertexFormat::BYTE by;
          VertexFormat::UBYTE uby;
          VertexFormat::SHORT sh;
          VertexFormat::USHORT ush;
          VertexFormat::INT in;
          VertexFormat::UINT uin;
          VertexFormat::FLOAT flo;
          VertexFormat::DOUBLE dou;

          char ch[sizeof (VertexFormat::DOUBLE)];
        };

        if (vi == vertex.end ())
          throw std::logic_error ("Vertex has less elements than format");

        switch (spec.type)
        {
          case VertexFormat::DATA_TYPE_BYTE:
            by = *vi++;
            break;
          case VertexFormat::DATA_TYPE_UBYTE:
            uby = *vi++;
            break;
          case VertexFormat::DATA_TYPE_SHORT:
            sh = *vi++;
            break;
          case VertexFormat::DATA_TYPE_USHORT:
            ush = *vi++;
            break;
          case VertexFormat::DATA_TYPE_INT:
            in = *vi++;
            break;
          case VertexFormat::DATA_TYPE_UINT:
            uin = *vi++;
            break;
          case VertexFormat::DATA_TYPE_FLOAT:
            flo = *vi++;
            break;
          case VertexFormat::DATA_TYPE_DOUBLE:
            dou = *vi++;
            break;
          default:
            assert (false);
        }

        part.insert (part.end (), ch,
            ch + VertexFormat::size_of_type (spec.type));
      }
    }

    if (vi != vertex.end ())
      throw std::logic_error ("Vertex has more elements than format");

    if (priv->static_buf)
      priv->static_buf->write (-1, &static_part[0]);

    if (priv->dynamic_buf)
      priv->dynamic_buf->write (-1, &dynamic_part[0]);

    return priv->index++;
  }

  unsigned
  Mesh::add_index (const VertexDataElement &elem)
  {
    if (elem.get_type () != priv->index_type)
      throw std::logic_error ("index type mismatch");

    union
    {
      VertexFormat::UBYTE b;
      VertexFormat::USHORT s;
      VertexFormat::UINT i;
    };

    b = s = i = 0;

    switch (priv->index_type)
    {
      case VertexFormat::DATA_TYPE_UBYTE:
        b = elem;
        break;
      case VertexFormat::DATA_TYPE_USHORT:
        s = elem;
        break;
      case VertexFormat::DATA_TYPE_UINT:
        i = elem;
        break;
      default:
        assert (false);
    }

    if (i >= priv->index)
      throw std::out_of_range ("index larger than amount of vertices");

    switch (priv->index_type)
    {
      case VertexFormat::DATA_TYPE_UBYTE:
        priv->indices->write (-1, b);
        break;
      case VertexFormat::DATA_TYPE_USHORT:
        priv->indices->write (-1, s);
        break;
      case VertexFormat::DATA_TYPE_UINT:
        priv->indices->write (-1, i);
        break;
      default:
        assert (false);
    }

    return priv->serial++;
  }

  namespace
  {
    void
    disable_gl_arrays (GLExtensionProxy &proxy)
    {
      const GLenum to_disable[] =
      {
        GL_VERTEX_ARRAY,
        GL_NORMAL_ARRAY,
        GL_COLOR_ARRAY,
        GL_TEXTURE_COORD_ARRAY,
        GL_EDGE_FLAG_ARRAY,
        GL_INDEX_ARRAY,
        GL_INVALID_ENUM
      };

      for (const GLenum *i = to_disable; *i != GL_INVALID_ENUM; i++)
      {
        glDisableClientState (*i);
      }

      /* FIXME add extension arrays (like vertexattrib... before using them */
    }
  }

  namespace
  {
    GLenum
    to_gl_mode (Mesh::RenderMode mode)
    {
      switch (mode)
      {
        case Mesh::RENDER_MODE_POINT_LIST:
          return GL_POINTS;
        case Mesh::RENDER_MODE_LINE_LIST:
          return GL_LINES;
        case Mesh::RENDER_MODE_LINE_STRIP:
          return GL_LINE_STRIP;
        case Mesh::RENDER_MODE_TRIANGLE_LIST:
          return GL_TRIANGLES;
        case Mesh::RENDER_MODE_TRIANGLE_STRIP:
          return GL_TRIANGLE_STRIP;
        case Mesh::RENDER_MODE_QUAD_LIST:
          return GL_QUADS;
        case Mesh::RENDER_MODE_QUAD_STRIP:
          return GL_QUAD_STRIP;
        case Mesh::RENDER_MODE_SINGLE_POLYGON:
          return GL_POLYGON;
        default:
          assert (false);
          return GL_INVALID_ENUM;
      }
    }
  }

  void
  Mesh::render (RenderMode mode,
                VertexFormat::UINT start_index,
                size_t count)
  {
    if (start_index >= priv->index)
      throw std::out_of_range ("index > amount of vertices in mesh");

    if (count > priv->index - start_index)
      count = priv->index - start_index;

    disable_gl_arrays (*(priv->proxy));

    maybe_bind_vertices ();

    glDrawArrays (to_gl_mode (mode), start_index, count);

    disable_gl_arrays (*(priv->proxy));
  }

  void
  Mesh::render_indexed (RenderMode mode,
                        unsigned start_serial,
                        size_t count)
  {
    if (start_serial >= priv->serial)
      throw std::out_of_range ("serial > amount of indices in mesh");

    if (count > priv->serial - start_serial)
      count = priv->serial - start_serial;

    disable_gl_arrays (*(priv->proxy));

    maybe_bind_vertices ();
    maybe_bind_indices ();

    glDrawElements (to_gl_mode (mode), count,
        VertexFormat::to_gl_type (priv->index_type),
        priv->indices->get_address_of (start_serial));

    disable_gl_arrays (*(priv->proxy));
  }

  void
  Mesh::maybe_bind_vertices ()
  {
    size_t static_offset = 0;
    size_t dynamic_offset = 0;

    for (VertexFormat::const_iterator it = priv->vf.begin ();
        it != priv->vf.end (); it++)
    {
      const VertexFormat::DataMemberSpec &spec = *it;

      boost::shared_ptr<VertexDataBuffer> buf =
        spec.dynamic ? priv->dynamic_buf : priv->static_buf;
      
      const void *base =
        buf->get_address_of (0);
      
      GLenum gl_type =
        VertexFormat::to_gl_type (spec.type);
      
      GLsizei gl_stride =
        spec.dynamic ? priv->dynamic_size : priv->static_size;

      size_t &offset =
        spec.dynamic ? dynamic_offset : static_offset;

      const GLvoid *gl_ptr =
        (const GLvoid *) ((char *) base + offset);

      assert (buf != NULL);

      if (bound_vertices != priv.get ())
        buf->bind ();

      switch (spec.role)
      {
        case VertexFormat::DATA_ROLE_POS:

          if (bound_vertices != priv.get ())
            glVertexPointer (spec.count, gl_type, gl_stride, gl_ptr);
          glEnableClientState (GL_VERTEX_ARRAY);
          break;

        case VertexFormat::DATA_ROLE_NORMAL:

          if (bound_vertices != priv.get ())
            glNormalPointer (gl_type, gl_stride, gl_ptr);
          glEnableClientState (GL_NORMAL_ARRAY);
          break;

        case VertexFormat::DATA_ROLE_COLOR:

          if (bound_vertices != priv.get ())
            glColorPointer (spec.count, gl_type, gl_stride, gl_ptr);
          glEnableClientState (GL_COLOR_ARRAY);
          break;

        default:

          SS_WARN ("unhandled role %u, implement it NOW", spec.role);
          break;
      }

      offset += spec.count * VertexFormat::size_of_type (spec.type);
    }

    bound_vertices = priv.get ();
  }

  void
  Mesh::maybe_bind_indices ()
  {
    if (bound_indices == priv.get())
      return;

    priv->indices->bind ();

    bound_indices = priv.get();
  }
}

