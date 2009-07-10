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

#ifndef __SS_MESH_H_
#define __SS_MESH_H_

#include "VertexFormat.h"

#include <vector>

#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace SilkyStrings
{
  struct VertexDataBufferManager;
  struct GLExtensionProxy;

  /** The Mesh class represents hunks of geometry data.
   *
   * A mesh is a collection of vertices which share the same vertex format and
   * which are likely to be drawn together. It may also contain vertex indices.
   */

  class Mesh
  {
    public:

      /** Constructor.
       *
       * Throws if the format is incomplete or if index_type is not one of
       * VertexFormat::{DATA_TYPE_UBYTE,DATA_TYPE_USHORT,DATA_TYPE_UINT}
       *
       * @param format The vertex format for the mesh. It must be complete,
       * ie. it must have at least a member with role DATA_ROLE_POS.
       *
       * @param mgr A valid VertexDataBufferManager instance.
       * @param proxy A valid GLExtensionProxy instance.
       *
       * @param index_type The type of vertex indices to use for this mesh.
       *
       * @param dynamic_indices Specifies if the mesh should be optimized for
       * dynamic vertex index updating. The optimization direction for vertex
       * data is specified in the format.
       */

      Mesh (const VertexFormat &format,
            boost::weak_ptr<VertexDataBufferManager> mgr,
            boost::shared_ptr<GLExtensionProxy> proxy,
            VertexFormat::DataType index_type = VertexFormat::DATA_TYPE_USHORT,
            bool dynamic_indices = false);

      /** Add a vertex to the mesh.
       *
       * Add a vertex to this mesh. The vertex must match the mesh's vertex
       * format.
       *
       * Example:
       *
       * The vertex format is:
       * [(DATA_ROLE_POS, DATA_TYPE_FLOAT, 3),
       *  (DATA_ROLE_COLOR, DATA_TYPE_UBYTE, 4]
       * 
       * When inserting vertices, the /vertex/ must have three
       * VertexFormat::FLOATs describing the position of the vertex followed by
       * four VertexFormat::UBYTEs describing the color of the vertex.
       *
       * Returns the index of the added vertex, which can be used with
       * add_index() and render(). Although it is an UINT, it can be casted to
       * a smaller index type and used with add_index() if the mesh's index type
       * given at construction time so requires.
       *
       * Throws if the vertex doesn't match the mesh's format.
       *
       * @param vertex The vertex to insert. Must be in the correct format.
       * @return The vertex index of the added vertex.
       */

      VertexFormat::UINT
      add_vertex (const std::vector<VertexDataElement> &vertex);

      /** Add a vertex index to the mesh.
       *
       * Add an index to this mesh. Indices are used when rendering using
       * render_indices(). Valid vertex indices are returned by add_vertex().
       *
       * Throws if /index/ is not in this mesh's index type specified at
       * construction time. Also throws if /index/ is higher than the amount
       * of vertices in this mesh. This means you must add vertices first and
       * only after that add the indices addressing them.
       *
       * @param index The vertex index to add.
       * @return The serial number of the added index element, which can be
       * passed to render_indexed.
       */

      unsigned
      add_index (const VertexDataElement &index);

      /**
       * The RenderMode enum specifies the primitive type and the way to
       * interpret the geometry data while drawing.
       */

      enum RenderMode
      {
        RENDER_MODE_POINT_LIST,
        RENDER_MODE_LINE_LIST, 
        RENDER_MODE_LINE_STRIP,
        RENDER_MODE_TRIANGLE_LIST,
        RENDER_MODE_TRIANGLE_STRIP,
        RENDER_MODE_TRIANGLE_FAN,
        RENDER_MODE_QUAD_LIST,
        RENDER_MODE_QUAD_STRIP,
        RENDER_MODE_SINGLE_POLYGON
      };

      /** Render the mesh without using vertex indices.
       *
       * @param mode How to interpret the vertex data and what to draw.
       * @param start_index Index of the first vertex that should be drawn.
       * @param count How many vertices should be drawn.
       */

      void
      render (RenderMode mode = RENDER_MODE_TRIANGLE_LIST,
              VertexFormat::UINT start_index = 0,
              size_t count = ~0);

      /** Render the mesh using vertex indices.
       *
       * @param mode How to interpret the vertex data and what to draw.
       * @param start_serial Serial of the first index that should be used.
       * @param count How many vertices should be drawn.
       */

      void
      render_indexed (RenderMode mode = RENDER_MODE_TRIANGLE_LIST,
                      unsigned start_serial = 0,
                      size_t count = ~0);

      /**
       * @todo Think about dynamic updating of meshes sometime soon
       * (needed for any cool particle FX)
       */

      /** Comparison operator.
       *
       * Can be used to sort meshes for minimal mesh state changes.
       * Copies of this mesh compare as equal, others as inequal.
       *
       * @param a A Mesh object to compare to.
       * @return The result of the comparison.
       */

      bool
      operator< (const Mesh &a) const;

    private:

      void maybe_bind_vertices ();
      void maybe_bind_indices ();

      struct Private;
      static Private *bound_vertices;
      static Private *bound_indices;
      boost::shared_ptr<Private> priv;
  };
}

#endif

