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

#ifndef __SS_TEXTURE2D_H_
#define __SS_TEXTURE2D_H_

#include <boost/shared_ptr.hpp>

namespace SilkyStrings
{
  struct GLExtensionProxy;

  /**
   * The Texture2D class represents a OpenGL two-dimensional texture.
   */

  class Texture2D
  {
    public:

      /**
       * Pixel role and data type of the texture.
       */

      enum Format
      {
        FORMAT_GREYSCALE_UBYTE,
        FORMAT_ALPHA_UBYTE,
        FORMAT_RGB_UBYTE,
        FORMAT_RGBA_UBYTE
        /** @todo add more */
      };

      /**
       * Constructor.
       *
       * @param proxy A valid GLExtensionProxy instance.
       * @param format Pixel role and data type of the texture.
       * @param clamp If clamping should be used on texture coordinates for this
       * texture. The default is repeating.
       * @param width_log2 Base-2 logarithm of the initial width of the texture.
       * @param height_log2 Base-2 logarithm of the initial height of the
       * texture.
       * @param mipmap If mipmapping should be used with the texture. Almost
       * always a good idea.
       */

      Texture2D (boost::shared_ptr<GLExtensionProxy> proxy,
                 Format format,
                 bool clamp = false,
                 unsigned width_log2 = 0,
                 unsigned height_log2 = 0,
                 bool mipmap = true);

      /** Bind the texture.
       *
       * Binds the texture to the currently active texture unit.
       */

      void
      bind ();

      /** Blit image data on the texture.
       *
       * Resizes this texture if needed so that the blitted surface fits in.
       *
       * The surface pixel format must match the texture format specified on
       * object construction. The surface must be in row-major order.
       *
       * If specified, stride specifies a offset to add to the address of a row
       * in the source surface to go up one row.
       *
       * May invalidate any bind() state.
       *
       * @param x X coordinate to blit to.
       * @param y Y coordinate to blit to.
       * @param width Width of the surface to blit.
       * @param height Height of the surface to blit.
       * @param data Pointer to the lowermost row of the surface to blit.
       * @param stride Address difference of successive rows, going upwards.
       */

      void
      blit (unsigned x,
            unsigned y,
            unsigned width,
            unsigned height,
            unsigned char *data,
            ptrdiff_t stride = 0);

      /** Translate pixel coordinates to normalized texture coordinates.
       *
       * Stores to tex_x and tex_y the texture coordinates corresponding to
       * the given pixel coordinates in this texture. blit() may invalidate
       * them.
       *
       * The resulting normalized coordinates can be used as GL texture
       * coordinates when drawing with this texture.
       *
       * @param tex_x Normalized X texture coordinate to fill in.
       * @param tex_y Normalized Y texture coordinate to fill in.
       * @param x X pixel coordinate.
       * @param y Y pixel coordinate.
       */

      void
      get_tex_coord (float &tex_x,
                     float &tex_y,
                     unsigned x,
                     unsigned y);

    private:

      void
      maybe_upload ();

      struct Private;
      boost::shared_ptr<Private> priv;
  };
}

#endif

