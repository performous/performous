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

#ifndef __SS_MESHFACTORY_H_
#define __SS_MESHFACTORY_H_

#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "Mesh.h"
#include "VertexFormat.h"

namespace SilkyStrings
{
  struct VertexDataBufferManager;
  struct GLExtensionProxy;

  /**
   * The MeshFactory class provides Mesh objects representing certain game
   * elements.
   */

  class MeshFactory
  {
    public:

      /** Constructor.
       *
       * @param mgr A valid VertexDataBufferManager instance.
       * @param proxy A valid GLExtensionProxy instance.
       * @param head_tessel The amount of slices to tessellate the chord head
       * geometry to.
       * @param tail_tessel The amount of slices to tessellate the chord tail
       * geometry to.
       */

      MeshFactory (boost::weak_ptr<VertexDataBufferManager> mgr,
                   boost::shared_ptr<GLExtensionProxy> proxy,
                   unsigned head_tessel = 25,
                   unsigned tail_tessel = 12);

      /**
       * Create a Mesh representing a chord head.
       *
       * @return A ready-to-draw Mesh instance.
       */

      Mesh
      create_head ();

      /**
       * Create a Mesh representing a chord tail.
       *
       * @return A ready-to-draw Mesh instance.
       */

      Mesh
      create_tail ();

    private:

      Mesh head, tail;
  };
}

#endif

