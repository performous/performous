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

#ifndef __SS_SOFTWAREVERTEXDATABUFFER_H_
#define __SS_SOFTWAREVERTEXDATABUFFER_H_

#include "VertexDataBuffer.h"

#include <vector>

#include <boost/shared_ptr.hpp>

namespace SilkyStrings
{
  struct GLExtensionProxy;

  /** Software implementation of VertexDataBuffer
   *
   * The SoftwareVertexDataBuffer class implements VertexDataBuffer entirely
   * without hardware support. It is used if nothing more efficient is
   * available, and as backing store for volatile and write-only buffers.
   */

  class SoftwareVertexDataBuffer : public VertexDataBuffer
  {
    public:

      /** Constructor.
       *
       * @param proxy A valid GLExtensionProxy instance.
       * @param glob_size Size of an element glob, in basic machine units.
       */

      inline SoftwareVertexDataBuffer (boost::shared_ptr<GLExtensionProxy> proxy, size_t glob_size)
        : proxy(proxy), glob_size(glob_size) {}

      void
      write (int index, void *data);

      /** Read back data from the buffer.
       *
       * Operation unique to SoftwareVertexDataBuffer - readback.
       * Software buffers implement readback efficiently, so they can be used
       * as backing store for buffers which can be efficiently only written to.
       *
       * Reads n globs of data staring from the glob at the given index,
       * storing the data to the memory area pointed by /data/. If /n/ < 0, the
       * rest of the buffer is read.
       *
       * May throw if index < 0 or index > the amount of stored globs.
       * May also throw if index + n is more than the amount of stored globs.
       * May also throw if data is invalid.
       *
       * @param index The index of the first glob to read back.
       * @param n The amount of globs to read back.
       * @param data A buffer to read the data back to.
       */

      void
      read (int index, int n, void *data);

      void
      bind ();

      const void *
      get_address_of (int index) const;

    private:

      boost::shared_ptr<GLExtensionProxy> proxy;
      size_t glob_size;
      std::vector<char> mem;
  };
}

#endif

