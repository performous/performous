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

#ifndef __SS_GLEXTENSIONPROXY_H_
#define __SS_GLEXTENSIONPROXY_H_

#include <boost/shared_ptr.hpp>

#include "GL.h"

namespace SilkyStrings
{
  struct WM;

  /**
   * The GLExtensionProxy class provides services for using OpenGL extensions.
   */

  class GLExtensionProxy
  {
    public:

      /** Constructor.
       *
       * Constructs a GLExtensionProxy object.
       *
       * @param wm A valid WM instance.
       */

      GLExtensionProxy (boost::shared_ptr<WM> wm);

      /*
       * GL_ARB_vertex_buffer_object
       */

      /**
       * Query for GL_ARB_vertex_buffer_object support.
       *
       * @return If VBO is supported.
       */

      bool
      has_vbo ();

      /**
       * Create a new VBO buffer object.
       *
       * @return Handle of the new buffer object.
       */

      GLuint
      new_buffer ();

      /**
       * Destroy a VBO buffer object.
       *
       * @param buffer Handle of the buffer object to destroy.
       */

      void
      delete_buffer (GLuint buffer);

      /**
       * Bind a VBO buffer object.
       *
       * @param target The buffer object target to bind the buffer object to.
       * @param buffer Handle of the buffer object to bind.
       */

      void
      bind_buffer (GLenum target, GLuint buffer);

      /**
       * Upload data to a VBO buffer object.
       *
       * @param target The buffer object target to upload data to.
       * @param size The amount of data to upload, in basic machine units.
       * @param data Pointer to the data to upload.
       * @param usage Usage hint for the GL to optimize the buffer object.
       */

      void
      buffer_upload (GLenum target,
                     GLsizeiptrARB size,
                     const void *data,
                     GLenum usage);

      /**
       * Update a region of a VBO buffer object.
       *
       * @param target The buffer object target to update.
       * @param offset Offset to the region from the beginning of the buffer.
       * @param size Size of the region, in basic machine units.
       * @param data Pointer to the data to update with.
       */

      void
      buffer_update (GLenum target,
                     GLintptrARB offset,
                     GLsizeiptrARB size,
                     const void *data);

      /**
       * Download data from a region of a VBO buffer object.
       *
       * @param target The buffer object target to download data from.
       * @param offset Offset of the region to download from.
       * @param size Size of the region to download from, in bytes.
       * @param data Buffer to download the data to.
       */

      void
      buffer_download (GLenum target,
                       GLintptrARB offset,
                       GLsizeiptrARB size,
                       void *data);

      /* go map your socks */

      /*
       * GL_ARB_multitexture
       */

      /**
       * Query for GL_ARB_multitexture support.
       *
       * @return If GL_ARB_multitexture is supported.
       */

      bool
      has_multitex ();

      /**
       * Activate a texture unit as a target of server state texture commands.
       *
       * @param tex The texture unit to activate.
       */

      void
      activate_texture (GLenum tex);

      /**
       * Activate a texture unit as a target of client state texture commands.
       *
       * @param tex The texture unit to activate.
       */

      void
      client_activate_texture (GLenum tex);

      /**
       * Specify the current homogenious texture coordinates for a texture unit.
       *
       * This function works even if multitexturing isn't supported if tex is
       * GL_TEXTURE0_ARB.
       *
       * @param tex The texture unit to modify.
       * @param s The new S texture coordinate.
       * @param t The new T texture coordinate.
       * @param r The new R texture coordinate.
       * @param q The new Q texture coordinate.
       */

      void
      multitex_coord (GLenum tex,
                      GLfloat s,
                      GLfloat t = 0.0f,
                      GLfloat r = 0.0f,
                      GLfloat q = 1.0f); 

      /*
       * GL_SGIS_generate_mipmap
       */

      /**
       * Query for GL_SGIS_generate_mipmap.
       *
       * @return If GL_SGIS_generate_mipmap is supported.
       */

      bool
      has_generate_mipmap ();

      /**
       * Query for GL_EXT_rescale_normal.
       *
       * @return If GL_EXT_rescale_normal is supported.
       */

      bool
      has_rescale_normal ();

    private:

      struct Private;
      boost::shared_ptr<Private> priv;
  };
}

#endif

