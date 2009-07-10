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

#include "TextRenderer.h"
#include "Mesh.h"
#include "VertexFormat.h"
#include "GL.h"
#include "GLExtensionProxy.h"
#include "Texture2D.h"
#include "Util.h"

#include <vector>
#include <map>
#include <utility>
#include <stdexcept>
#include <algorithm>

#include <boost/shared_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

namespace SilkyStrings
{
  namespace
  {
    class GLDisplayList
    {
      public:

        inline GLDisplayList ()
        {
          dl = glGenLists (1);
        }

        inline ~GLDisplayList ()
        {
          glDeleteLists (dl, 1);
        }

        inline operator GLuint ()
        {
          return dl;
        }

      private:

        GLDisplayList (const GLDisplayList &);
        void operator= (const GLDisplayList &a);

        GLuint dl;
    };

    struct Text
    {
      Texture2D tex;
      boost::shared_ptr<GLDisplayList> dl;

      inline Text (Texture2D tex, boost::shared_ptr<GLDisplayList> dl)
        : tex (tex), dl(dl)
      {
      }
    };

    class FTLibrary
    {
      public:

        FTLibrary ();
        ~FTLibrary ();
        
        inline operator FT_Library ()
        {
          return lib;
        }

      private:

        static unsigned refcount;
        static FT_Library lib;
    };

    unsigned FTLibrary::refcount = 0;
    FT_Library FTLibrary::lib = NULL;

    FTLibrary::FTLibrary ()
    {
      if (!refcount++)
        if (FT_Init_FreeType (&lib))
          throw std::runtime_error ("Failed to initialize FreeType2");
    }
    
    FTLibrary::~FTLibrary ()
    {
      if (!--refcount)
      {
        if (FT_Done_FreeType (lib))
          SS_WARN ("FreeType2 deinit failed ?!");
        lib = NULL;
      }
    }
  }

  struct TextRenderer::Private
  {
    std::map<std::string, Text> cache;
    boost::shared_ptr<GLExtensionProxy> proxy;
    FTLibrary lib;
    bool kern;

    unsigned height;

    FT_Face ft_face;

    struct Glyph
    {
      FT_Glyph ft_glyph;
      FT_UInt glyph_index;

      Glyph ()
      {
        ft_glyph = FT_Glyph (NULL);
        glyph_index = 0;
      }

      ~Glyph ()
      {
        if (ft_glyph)
          FT_Done_Glyph (ft_glyph);
      }
    };

    std::vector<Glyph> glyphs;

    inline Private (boost::shared_ptr<GLExtensionProxy> proxy)
      : proxy(proxy)
    {
      ft_face = NULL;
    }

    inline ~Private ()
    {
      if (ft_face)
        FT_Done_Face (ft_face);
    }
  };

  TextRenderer::TextRenderer (const std::string &font,
                              unsigned height,
                              boost::shared_ptr<GLExtensionProxy> proxy)
    : priv (new Private (proxy))
  {
    priv->height = 0;

    FT_Error err;
    err = FT_New_Face (priv->lib,
                       font.c_str (),
                       0,
                       &priv->ft_face);

    if (err)
    {
      throw std::logic_error ("Failed to load font");
    }

    err = FT_Set_Pixel_Sizes (priv->ft_face,
                              0,
                              height);

    if (err)
    {
      throw std::logic_error ("Failed to set font height");
    }

    priv->glyphs.resize (256);
    priv->kern = FT_HAS_KERNING (priv->ft_face);

    FT_UInt glyph_index;
    for (FT_ULong char_code = FT_Get_First_Char (priv->ft_face, &glyph_index);
          glyph_index != 0;
          char_code = FT_Get_Next_Char (priv->ft_face, char_code, &glyph_index))
    {
      err = FT_Load_Glyph (priv->ft_face, glyph_index, FT_LOAD_DEFAULT);

      if (err)
      {
        SS_WARN ("Ignoring char %c because of Freetype error: %d", (char) char_code, err);
        continue;
      }

      if (char_code >= 256)
        continue;

      FT_Glyph tmp;
      err = FT_Get_Glyph (priv->ft_face->glyph, &tmp);

      if (err)
      {
        SS_WARN ("Ignoring char %c because of Freetype error: %d", (char) char_code, err);
        continue;
      }

      err = FT_Glyph_To_Bitmap (&tmp,
          FT_RENDER_MODE_LIGHT, 0, 1);

      if (err)
      {
        SS_WARN ("Ignoring char %c because of Freetype error: %d", (char) char_code, err);
        continue;
      }

      priv->glyphs[char_code].ft_glyph = tmp;
      priv->glyphs[char_code].glyph_index = glyph_index;
    }
  }

  void
  TextRenderer::render (const std::string &_text)
  {
    if (!_text.length ())
      return;

    if (priv->cache.find (_text) == priv->cache.end ())
      populate (_text);

    if (priv->proxy->has_multitex ())
      priv->proxy->activate_texture (GL_TEXTURE0_ARB);

    Text &text = priv->cache.find (_text)->second;

    priv->proxy->activate_texture (GL_TEXTURE0_ARB);
    text.tex.bind ();

    glCallList (*text.dl);
  }

  void
  TextRenderer::discard (const std::string &text)
  {
    priv->cache.erase (text);
  }

  void
  TextRenderer::populate (const std::string &text)
  {
    Text cached_text =
      Text (Texture2D (priv->proxy, Texture2D::FORMAT_ALPHA_UBYTE, true),
            boost::shared_ptr<GLDisplayList> (new GLDisplayList ()));

    unsigned curr_x = 0;
    unsigned char prev = 0;
    unsigned height = 0;
    for (unsigned i = 0; i < text.length (); i++)
    {
      unsigned char ch = text[i];

      if (!priv->glyphs[ch].ft_glyph)
      {
        SS_WARN ("unknown char %c", ch);
        continue;
      }

      if (prev && priv->kern)
      {
        FT_Vector d = {0, 0};

        FT_Error err = FT_Get_Kerning (priv->ft_face,
            priv->glyphs[prev].glyph_index,
            priv->glyphs[ch].glyph_index, FT_KERNING_DEFAULT, &d);

        if (err)
        {
          SS_WARN ("failed to get kerning due to FreeType error %d", err);
          d.x = d.y = 0;
        }

        curr_x += d.x / 64;
      }

      FT_BitmapGlyph glyph = FT_BitmapGlyph (priv->glyphs[ch].ft_glyph);

      ptrdiff_t stride = -glyph->bitmap.pitch;
      unsigned char *buf =
        glyph->bitmap.buffer + glyph->bitmap.pitch * (glyph->bitmap.rows - 1);

      if (stride >= 0)
      {
        buf = glyph->bitmap.buffer;
      }

      cached_text.tex.blit (curr_x + glyph->left, 0, glyph->bitmap.width,
          glyph->bitmap.rows, buf, stride);

      height = std::max (height, unsigned (glyph->bitmap.rows));

      curr_x += priv->glyphs[ch].ft_glyph->advance.x / 0xffff;

      prev = ch;
    }

    priv->height = std::max (height, priv->height);

    float height_ratio = 0.5f * float (height) / float (priv->height);
    float width = 0.5f * float (curr_x) / float (priv->height);

    float tex_x, tex_y;
    cached_text.tex.get_tex_coord (tex_x, tex_y, curr_x, height);
    
    glNewList (*cached_text.dl, GL_COMPILE);

    glPushAttrib (GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    glEnable (GL_TEXTURE_2D);

    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_BLEND);

    glDisable (GL_LIGHTING);

    glBegin (GL_QUADS);
    {
      glTexCoord2f (0, 0);
      glVertex2f (-width, -height_ratio);

      glTexCoord2f (tex_x, 0);
      glVertex2f (width, -height_ratio);

      glTexCoord2f (tex_x, tex_y);
      glVertex2f (width, height_ratio);

      glTexCoord2f (0, tex_y);
      glVertex2f (-width, height_ratio);
    }
    glEnd ();

    glPopAttrib ();

    glEndList ();

    priv->cache.insert (std::make_pair (text, cached_text));
  }
}

