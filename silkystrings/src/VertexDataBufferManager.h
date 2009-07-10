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

#ifndef __SS_VERTEXDATABUFFERMANAGER_H_
#define __SS_VERTEXDATABUFFERMANAGER_H_

#include "VertexFormat.h"

#include <boost/shared_ptr.hpp>

namespace SilkyStrings
{
  struct VertexDataBuffer;
  struct ElementDataBuffer;
  struct GLExtensionProxy;

  /**
   * The VertexDataBufferManager creates the most optimal models of
   * {Vertex,Element}DataBuffer depending on hardware support.
   */

  class VertexDataBufferManager
  {
    public:

      /** Constructor.
       *
       * @param proxy A valid GLExtensionProxy instance.
       */

      VertexDataBufferManager (boost::shared_ptr<GLExtensionProxy> proxy);

      /** Destructor.
       *
       * Buffers created with this manager don't necessarily remain
       * valid after destruction.
       */

      ~VertexDataBufferManager ();

      /**
       * Creates the best possible model of VertexDataBuffer, valid for
       * rendering with the associated extension proxy. Behaviour of using it
       * after destroying this object is undefined.
       *
       * @param glob_size The size of an element glob in basic machine units.
       * @param dynamic If the buffer should be optimized for dynamic updating.
       */

      boost::shared_ptr<VertexDataBuffer>
      create_vertex_data_buffer (size_t glob_size,
                                 bool dynamic = false);

      /**
       * Creates the best possible model of ElementDataBuffer, valid for
       * rendering with the associated extension proxy. Behaviour of using it
       * after destroying this object is undefined.
       *
       * @param type The index data type for the new buffer.
       * @param dynamic If the buffer should be optimized for dynamic updating.
       */

      boost::shared_ptr<ElementDataBuffer>
      create_element_data_buffer (VertexFormat::DataType type,
                                  bool dynamic = false);

    private:

      boost::shared_ptr<GLExtensionProxy> ext;
  };
};  

#endif
