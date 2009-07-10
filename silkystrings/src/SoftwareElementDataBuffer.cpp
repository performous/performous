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

#include "SoftwareElementDataBuffer.h"
#include "VertexFormat.h"
#include "SoftwareVertexDataBuffer.h"
#include "GLExtensionProxy.h"
#include "GL.h"

#include <cassert>

#include <stdexcept>

namespace SilkyStrings
{
  SoftwareElementDataBuffer::SoftwareElementDataBuffer (boost::shared_ptr<GLExtensionProxy> proxy, VertexFormat::DataType type)
      : proxy(proxy), type(type),
        buf(new SoftwareVertexDataBuffer (proxy, VertexFormat::size_of_type (type)))
  {
    switch (type)
    {
      case VertexFormat::DATA_TYPE_UBYTE:
      case VertexFormat::DATA_TYPE_USHORT:
      case VertexFormat::DATA_TYPE_UINT:
        break;
      default:
        throw std::logic_error ("unsupported index type");
    }
  }

  void
  SoftwareElementDataBuffer::write (int i, const VertexDataElement &elem)
  {
    union
    {
      VertexFormat::UBYTE byte_val;
      VertexFormat::USHORT short_val;
      VertexFormat::UINT int_val;
    };

    if (elem.get_type () != type)
      throw std::logic_error ("elem has wrong type");

    switch (type)
    {
      case VertexFormat::DATA_TYPE_UBYTE:

        byte_val = elem;
        return buf->write (i, (void *) &byte_val);

      case VertexFormat::DATA_TYPE_USHORT:

        short_val = elem;
        return buf->write (i, (void *) &short_val);

      case VertexFormat::DATA_TYPE_UINT:

        int_val = elem;
        return buf->write (i, (void *) &int_val);

      default:

        assert (false);
    }
  }

  void
  SoftwareElementDataBuffer::read (int i, int n, void *data)
  {
    return buf->read (i, n, data);
  }

  void
  SoftwareElementDataBuffer::bind ()
  {
    if (proxy->has_vbo ())
      return proxy->bind_buffer (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
  }

  const void *
  SoftwareElementDataBuffer::get_address_of (int index) const
  {
    return buf->get_address_of (index);
  }
}

