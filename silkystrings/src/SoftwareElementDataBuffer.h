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

#ifndef __SS_SOFTWAREELEMENTDATABUFFER_H_
#define __SS_SOFTWAREELEMENTDATABUFFER_H_

#include "ElementDataBuffer.h"
#include "VertexFormat.h"
#include <boost/shared_ptr.hpp>

namespace SilkyStrings
{
  struct SoftwareVertexDataBuffer;
  struct GLExtensionProxy;

  /** Software implementation of ElementDataBuffer
   *
   * The SoftwareElementDataBuffer class implements the ElementDataBuffer
   * interface entirely without hardware support.
   */

  class SoftwareElementDataBuffer : public ElementDataBuffer
  {
    public:

      /** Constructor.
       *
       * @param proxy A valid GLExtensionProxy instance.
       * @param type The type to use for the indices. The allowed types are
       * UBYTE, USHORT and UINT.
       */

      SoftwareElementDataBuffer (boost::shared_ptr<GLExtensionProxy> proxy, VertexFormat::DataType type);

      void
      write (int index, const VertexDataElement &vertex_index);

      /** Read back data from the buffer.
       *
       * Like SoftwareVertexDataBuffer, SoftwareElementDataBuffer implements
       * readback efficiently. 
       *
       * @param index The index of the first element to read back.
       * @param n The amount of elements to read back.
       * @param data Pointer to an array with elements of the type given at
       * construction. 
       */

      void
      read (int index, int n, void *data);

      void
      bind ();

      const void *
      get_address_of (int index) const;

    private:

      boost::shared_ptr<GLExtensionProxy> proxy;
      VertexFormat::DataType type;
      boost::shared_ptr<SoftwareVertexDataBuffer> buf;
  };
}

#endif

