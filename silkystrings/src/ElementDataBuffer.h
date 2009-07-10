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

#ifndef __SS_ELEMENTDATABUFFER_H_
#define __SS_ELEMENTDATABUFFER_H_

#include "VertexFormat.h"

#include <stdexcept>

#include <boost/shared_ptr.hpp>

namespace SilkyStrings
{
  struct GLExtensionProxy;

  /**
   * The ElementDataBuffer is an abstract class for classes which can be used to
   * store GL vertex index data for rendering.
   */

  class ElementDataBuffer
  {
    public:

      /**
       * Stub virtual destructor.
       */

      virtual ~ElementDataBuffer () {}

      /** Update an element.
       *
       * Updates the element at the given index with the given vertex index.
       * If index is negative, the data is appended to the end of the buffer.
       *
       * May throw if index is higher than the current amount of elements
       * stored, or if vertex_index is of different data type than the one
       * specified at construction.
       *
       * @param index The index of the element to update, or negative to append.
       * @param vertex_index The vertex index to write to the element.
       */

      virtual void
      write (int index, const VertexDataElement &vertex_index) = 0;

      /** Bind the buffer as the active buffer.
       *
       * Binds this buffer as the active buffer. Until some other buffer is
       * bound as the active buffer, all calls to glDraw(Range)Elements will
       * read indices from this buffer.
       */

      virtual void
      bind () = 0;

      /** Get the address of an element.
       *
       * Returns a pointer to the /index/:th element of this buffer. The
       * pointer can be passed as-is to glDraw(Range)Elements as the /indices/
       * parameter after this buffer has been selected with bind().
       *
       * The pointer might not be in the caller's address space, so it should
       * only be used for the aforementioned GL draw calls and only after
       * binding this buffer.
       *
       * Throws if index is negative or higher than the amount of elements
       * stored.
       *
       * @param index The index of the element to query the address of.
       * @return The address of the element.
       */

      virtual const void *
      get_address_of (int index) const = 0;
  };
}

#endif

