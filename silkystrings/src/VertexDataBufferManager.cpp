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

#include "VertexDataBufferManager.h"
#include "SoftwareVertexDataBuffer.h"
#include "SoftwareElementDataBuffer.h"
#include "GLExtensionProxy.h"

#include <boost/shared_ptr.hpp>

namespace SilkyStrings
{
  VertexDataBufferManager::VertexDataBufferManager (boost::shared_ptr<GLExtensionProxy> proxy)
    : ext(proxy)
  {
  }

  VertexDataBufferManager::~VertexDataBufferManager ()
  {
  }

  boost::shared_ptr<VertexDataBuffer>
  VertexDataBufferManager::create_vertex_data_buffer (size_t glob_size, bool dynamic)
  {
    /* currently just creates software buffers */
    return boost::shared_ptr<VertexDataBuffer> (new SoftwareVertexDataBuffer (ext, glob_size));
  }

  boost::shared_ptr<ElementDataBuffer>
  VertexDataBufferManager::create_element_data_buffer (VertexFormat::DataType type, bool dynamic)
  {
    /* currently just creates software buffers */
    return boost::shared_ptr<ElementDataBuffer> (new SoftwareElementDataBuffer (ext, type));
  }
}

