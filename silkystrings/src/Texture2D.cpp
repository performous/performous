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

#include "Texture2D.h"
#include "GL.h"
#include "GLExtensionProxy.h"
#include "Util.h"

#include <assert.h>
#include <vector>

namespace SilkyStrings
{
  struct Texture2D::Private
  {
    boost::shared_ptr<GLExtensionProxy> proxy;
    Format format;
    bool mipmap;
    bool clamp;

    GLuint gl_tex;
    std::vector<GLubyte> data;
    unsigned alloc_width;
    unsigned alloc_height;
    int dirty;

    Private (boost::shared_ptr<GLExtensionProxy> proxy)
      : proxy(proxy)
    {
      dirty = 2;

      glGenTextures (1, &gl_tex);
    }

    ~Private ()
    {
      glDeleteTextures (1, &gl_tex);
    }
  };

  namespace
  {
    size_t
    pixel_size (Texture2D::Format format)
    {
      size_t element_size;
      size_t element_count;

      switch (format)
      {
        case Texture2D::FORMAT_GREYSCALE_UBYTE:
        case Texture2D::FORMAT_ALPHA_UBYTE:
        case Texture2D::FORMAT_RGB_UBYTE:
        case Texture2D::FORMAT_RGBA_UBYTE:
          element_size = sizeof (GLubyte);
          break;
        default:
          assert (false);
          element_size = 0;
          break;
      }

      switch (format)
      {
        case Texture2D::FORMAT_GREYSCALE_UBYTE:
        case Texture2D::FORMAT_ALPHA_UBYTE:
          element_count = 1;
          break;
        case Texture2D::FORMAT_RGB_UBYTE:
          element_count = 3;
          break;
        case Texture2D::FORMAT_RGBA_UBYTE:
          element_count = 4;
          break;
        default:
          assert (false);
          element_count = 0;
          break;
      }

      return element_size * element_count;
    }
  }

  Texture2D::Texture2D (boost::shared_ptr<GLExtensionProxy> proxy,
                        Format format,
                        bool clamp,
                        unsigned x,
                        unsigned y,
                        bool mipmap)
    : priv (new Private (proxy))
  {
    priv->format = format;
    priv->mipmap = mipmap;
    priv->clamp = clamp;

    priv->alloc_width = 1 << x;
    priv->alloc_height = 1 << y;

    priv->data.resize (pixel_size (format) * (1 << x) * (1 << y));
    std::fill (priv->data.begin (), priv->data.end (), GLubyte (0));
  }

  void
  Texture2D::bind ()
  {
    glBindTexture (GL_TEXTURE_2D, priv->gl_tex);

    maybe_upload ();
  }

  namespace
  {
    struct GL2DTexFormat
    {
      GLenum internalformat;
      GLenum format;
      GLenum type;
    };

    GL2DTexFormat
    to_gl_format (Texture2D::Format format)
    {
      GL2DTexFormat ret = {GL_INVALID_ENUM, GL_INVALID_ENUM, GL_INVALID_ENUM};

      switch (format)
      {
        case Texture2D::FORMAT_GREYSCALE_UBYTE:
          ret.internalformat = GL_LUMINANCE8;
          ret.format = GL_LUMINANCE;
          ret.type = GL_UNSIGNED_BYTE;
          break;
        case Texture2D::FORMAT_ALPHA_UBYTE:
          ret.internalformat = GL_ALPHA8;
          ret.format = GL_ALPHA;
          ret.type = GL_UNSIGNED_BYTE;
          break;
        case Texture2D::FORMAT_RGB_UBYTE:
          ret.internalformat = GL_RGB8;
          ret.format = GL_RGB8;
          ret.type = GL_UNSIGNED_BYTE;
          break;
        case Texture2D::FORMAT_RGBA_UBYTE:
          ret.internalformat = GL_RGBA8;
          ret.format = GL_RGBA;
          ret.type = GL_UNSIGNED_BYTE;
          break;
        default:
          assert (false);
      }

      return ret;
    }
  }

  void
  Texture2D::maybe_upload ()
  {
    if (!priv->dirty)
      return;

    GL2DTexFormat gl_format = to_gl_format (priv->format);

    if (priv->dirty == 1)
    {
      glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, priv->alloc_width,
          priv->alloc_height, gl_format.format, gl_format.type, &priv->data[0]);
    }
    else if (priv->dirty == 2)
    {
      if (priv->clamp)
      {
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      }

      if (priv->mipmap)
      {
        if (priv->proxy->has_generate_mipmap ())
        {
          glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
          glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
              GL_LINEAR_MIPMAP_NEAREST);
          glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
              GL_LINEAR);
        }
        else
        {
          SS_WARN ("mipmap generation not supported - falling back to not "
                   "mipmapping - should implement emulation");
          glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
              GL_LINEAR);
          glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
      }
      else
      {
          glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
              GL_LINEAR);
          glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }

      glTexImage2D (GL_TEXTURE_2D, 0, gl_format.internalformat,
          priv->alloc_width, priv->alloc_height, 0, gl_format.format,
          gl_format.type, &priv->data[0]);
    }
    else
    {
      assert (false);
    }

    priv->dirty = 0;
  }

  namespace
  {
    unsigned
    log2_ceil (unsigned val)
    {
      for (unsigned i = 0; i < sizeof (unsigned) * 8; i++)
      {
        if ((unsigned (1) << i) >= val)
          return i;
      }

      assert (false);
      return 0;
    }
  }

  void
  Texture2D::blit (unsigned x,
                   unsigned y,
                   unsigned width,
                   unsigned height,
                   unsigned char *data,
                   ptrdiff_t stride)
  {
    if (stride == 0)
      stride = width;

    if (x + width > priv->alloc_width
        || y + height > priv->alloc_height)
    {
      Texture2D tmp (priv->proxy, priv->format, priv->clamp,
          std::max (log2_ceil (x + width), log2_ceil (priv->alloc_width)),
          std::max (log2_ceil (y + height), log2_ceil (priv->alloc_height)));

      tmp.blit (0, 0, priv->alloc_width, priv->alloc_height, &priv->data[0]);

      *this = tmp;

      return blit (x, y, width, height, data, stride);
    }

    size_t size = pixel_size (priv->format);
    for (unsigned row = 0; row < height; row++)
    {
      std::copy (data + row * stride,
          data + row * stride + width * size,
          &priv->data[(y + row) * priv->alloc_width * size + x * size]);
    }

    if (!priv->dirty)
      priv->dirty = 1;
  }

  void
  Texture2D::get_tex_coord (float &tex_x,
                            float &tex_y,
                            unsigned x,
                            unsigned y)
  {
    tex_x = (float (x) + 0.5f) / (priv->alloc_width + 1);
    tex_y = (float (y) + 0.5f) / (priv->alloc_height + 1);
  }
}

