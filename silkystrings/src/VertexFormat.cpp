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

#include "VertexFormat.h"
#include "Util.h"
#include "GL.h"

#include <cassert>
#include <set>
#include <numeric>
#include <algorithm>

namespace SilkyStrings
{
  size_t
  VertexFormat::size_of_type (DataType t)
  {
    switch (t)
    {
      case DATA_TYPE_BYTE:
      case DATA_TYPE_UBYTE:
        return sizeof (GLbyte);
      case DATA_TYPE_SHORT:
      case DATA_TYPE_USHORT:
        return sizeof (GLshort);
      case DATA_TYPE_INT:
      case DATA_TYPE_UINT:
        return sizeof (GLint);
      case DATA_TYPE_FLOAT:
        return sizeof (GLfloat);
      case DATA_TYPE_DOUBLE:
        return sizeof (GLdouble);
      default:
        throw std::logic_error ("invalid DataType passed");
    };
  }

  GLenum
  VertexFormat::to_gl_type (DataType t)
  {
    switch (t)
    {
      case DATA_TYPE_BYTE:
        return GL_BYTE;
      case DATA_TYPE_UBYTE:
        return GL_UNSIGNED_BYTE;
      case DATA_TYPE_SHORT:
        return GL_SHORT;
      case DATA_TYPE_USHORT:
        return GL_UNSIGNED_SHORT;
      case DATA_TYPE_INT:
        return GL_INT;
      case DATA_TYPE_UINT:
        return GL_UNSIGNED_INT;
      case DATA_TYPE_FLOAT:
        return GL_FLOAT;
      case DATA_TYPE_DOUBLE:
        return GL_DOUBLE;
      default:
        throw std::logic_error ("invalid DataType passed");
    }
  }

  bool
  VertexFormat::DataMemberSpec::is_valid () const
  {
    switch (role)
    {
      case DATA_ROLE_POS:
        
        if (type != DATA_TYPE_SHORT
            && type != DATA_TYPE_INT
            && type != DATA_TYPE_FLOAT
            && type != DATA_TYPE_DOUBLE)
          return false;

        if (count < 2 || count > 4)
          return false;

        return true;

      case DATA_ROLE_NORMAL:

        if (type != DATA_TYPE_BYTE
            && type != DATA_TYPE_SHORT
            && type != DATA_TYPE_INT
            && type != DATA_TYPE_FLOAT
            && type != DATA_TYPE_DOUBLE)
          return false;

        if (count != 3)
          return false;

        return true;

      case DATA_ROLE_COLOR:

        if (type != DATA_TYPE_BYTE
            && type != DATA_TYPE_UBYTE
            && type != DATA_TYPE_SHORT
            && type != DATA_TYPE_USHORT
            && type != DATA_TYPE_INT
            && type != DATA_TYPE_UINT
            && type != DATA_TYPE_FLOAT
            && type != DATA_TYPE_DOUBLE)
          return false;

        if (count < 3 && count > 4)
          return false;

        return true;

      case DATA_ROLE_SECONDARY_COLOR:

        if (type != DATA_TYPE_BYTE
            && type != DATA_TYPE_UBYTE
            && type != DATA_TYPE_SHORT
            && type != DATA_TYPE_USHORT
            && type != DATA_TYPE_INT
            && type != DATA_TYPE_UINT
            && type != DATA_TYPE_FLOAT
            && type != DATA_TYPE_DOUBLE)
          return false;

        if (count != 3)
          return false;

        return true;

      case DATA_ROLE_TEXCOORD0:
      case DATA_ROLE_TEXCOORD1:
      case DATA_ROLE_TEXCOORD2:
      case DATA_ROLE_TEXCOORD3:
      case DATA_ROLE_TEXCOORD4:
      case DATA_ROLE_TEXCOORD5:
      case DATA_ROLE_TEXCOORD6:
      case DATA_ROLE_TEXCOORD7:

        if (type != DATA_TYPE_SHORT
            && type != DATA_TYPE_INT
            && type != DATA_TYPE_FLOAT
            && type != DATA_TYPE_DOUBLE)
          return false;

        if (count < 1 || count > 4)
          return false;

        return true;

      case DATA_ROLE_FOG_COORD:

        if (type != DATA_TYPE_FLOAT
            && type != DATA_TYPE_DOUBLE)
          return false;

        if (count != 1)
          return false;

        return true;

      case DATA_ROLE_ATTRIB0:
      case DATA_ROLE_ATTRIB1:
      case DATA_ROLE_ATTRIB2:
      case DATA_ROLE_ATTRIB3:
      case DATA_ROLE_ATTRIB4:
      case DATA_ROLE_ATTRIB5:
      case DATA_ROLE_ATTRIB6:
      case DATA_ROLE_ATTRIB7:
      case DATA_ROLE_ATTRIB8:
      case DATA_ROLE_ATTRIB9:
      case DATA_ROLE_ATTRIB10:
      case DATA_ROLE_ATTRIB11:
      case DATA_ROLE_ATTRIB12:
      case DATA_ROLE_ATTRIB13:
      case DATA_ROLE_ATTRIB14:
      case DATA_ROLE_ATTRIB15:
      case DATA_ROLE_ATTRIB16:
      case DATA_ROLE_ATTRIB17:
      case DATA_ROLE_ATTRIB18:
      case DATA_ROLE_ATTRIB19:
      case DATA_ROLE_ATTRIB20:
      case DATA_ROLE_ATTRIB21:
      case DATA_ROLE_ATTRIB22:
      case DATA_ROLE_ATTRIB23:
      case DATA_ROLE_ATTRIB24:
      case DATA_ROLE_ATTRIB25:
      case DATA_ROLE_ATTRIB26:
      case DATA_ROLE_ATTRIB27:
      case DATA_ROLE_ATTRIB28:
      case DATA_ROLE_ATTRIB29:
      case DATA_ROLE_ATTRIB30:
      case DATA_ROLE_ATTRIB31:

        if (count < 1 || count > 4)
          return false;

        return true;

      default:

        throw std::logic_error ("invalid data role");
    }
  }

  namespace
  {
    struct role_eq
    {
      role_eq (const VertexFormat::DataMemberSpec &spec) : spec (spec) {}

      bool
      operator() (const VertexFormat::DataMemberSpec &a)
      {
        return spec.role == a.role;
      }

      const VertexFormat::DataMemberSpec &spec;
    };
  }

  void
  VertexFormat::insert (const DataMemberSpec &spec)
  {
    if (std::find_if (begin (), end (), role_eq (spec)) != members.end())
      throw std::logic_error ("tried to add a member with the same role twice");

    members.insert (spec);
  }

  static size_t
  sum_sizes (size_t curr, const VertexFormat::DataMemberSpec &b)
  {
    return curr + b.count * VertexFormat::size_of_type (b.type);
  }

  size_t
  VertexFormat::get_size () const
  {
    return std::accumulate (begin(), end(), size_t (0), sum_sizes);
  }

  VertexFormat::const_iterator
  VertexFormat::begin () const
  {
    return members.begin ();
  }

  VertexFormat::const_iterator
  VertexFormat::end () const
  {
    return members.end ();
  }
}

