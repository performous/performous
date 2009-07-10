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

#ifndef __SS_VERTEXDATABUFFER_H_
#define __SS_VERTEXDATABUFFER_H_

#include "VertexFormat.h"

#include <boost/shared_ptr.hpp>

namespace SilkyStrings
{
  struct GLExtensionProxy;

  /**
   * The VertexDataBuffer class is an abstract base class for classes which can
   * be used to store vertex data for rendering.
   */

  class VertexDataBuffer
  {
    public:

      /**
       * Stub virtual destructor.
       */

      virtual ~VertexDataBuffer () {}

      /** Update a element glob.
       *
       * Updates the element glob at the given index with the given data.
       * If index is negative, the data is appended to the end of the buffer.
       *
       * May throw if index is higher than the current amount of elements
       * stored.
       *
       * @param index The index of the glob to update.
       * @param data Pointer to data to update the glob with.
       */

      virtual void
      write (int index, void *data) = 0;

      /** Bind the buffer as the active buffer.
       *
       * Binds this buffer as the active buffer. Until some other buffer is
       * bound as the active buffer, all calls to gl*Pointer will refer to this
       * buffer.
       */

      virtual void
      bind () = 0;

      /** Get address of an element glob.
       *
       * Returns a pointer to the element at index. The pointer can be passed
       * to glDraw(Range}Arrays and gl*Pointer after binding this buffer.
       *
       * The pointer might not be in the caller's address space, so it should
       * only be used for the aforementioned GL draw calls and only after
       * binding this buffer.
       *
       * Throws if index is negative or higher than the amount of elements
       * stored.
       *
       * @param index The index of the glob to return the address of.
       * @return The address of the glob.
       */

      virtual const void *
      get_address_of (int index) const = 0;
  };
}

#endif

