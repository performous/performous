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

#include "SoftwareVertexDataBuffer.h"
#include "GLExtensionProxy.h"

#include <stdexcept>

#include <GL/glext.h>

namespace SilkyStrings
{
  void
  SoftwareVertexDataBuffer::write (int i, void *p)
  {
    if (p == NULL)
      throw std::logic_error ("NULL pointer");

    if (i < 0)
    {
      mem.insert (mem.end (), (char *) p, (char *) p + glob_size);
    }
    else
    {
      if (i * glob_size >= mem.size ())
        throw std::out_of_range ("overindex");

      std::copy ((char *) p, (char *) p + glob_size,
          mem.begin () + i * glob_size);
    }
  }

  void
  SoftwareVertexDataBuffer::read (int i, int n, void *p)
  {
    if (n < 0)
      n = mem.size() / glob_size - i;

    if (i < 0 || (i + n) * glob_size > mem.size())
      throw std::out_of_range ("overindex");

    if (p == NULL)
      throw std::logic_error ("NULL pointer");

    std::copy (mem.begin () + glob_size * i,
        mem.begin () + glob_size * (i + n), (char *) p);
  }

  void
  SoftwareVertexDataBuffer::bind ()
  {
	if (proxy->has_vbo ())
      proxy->bind_buffer (GL_ARRAY_BUFFER_ARB, 0);
  }

  const void *
  SoftwareVertexDataBuffer::get_address_of (int index) const
  {
    if (index < 0)
      throw std::out_of_range ("index < 0");
  
    if (index * glob_size >= mem.size ())
      throw std::out_of_range ("overindex");

    return &mem[0] + index * glob_size;
  }
}

