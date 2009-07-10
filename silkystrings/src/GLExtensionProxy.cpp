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

#include "GLExtensionProxy.h"
#include "WM.h"
#include "GL.h"

#include <stdexcept>
#include <cstdlib>

namespace SilkyStrings
{
  struct GLExtensionProxy::Private
  {
    bool vbo;
    PFNGLGENBUFFERSARBPROC gen_buffers;
    PFNGLDELETEBUFFERSARBPROC delete_buffers;
    PFNGLBINDBUFFERARBPROC bind_buffer;
    PFNGLBUFFERDATAARBPROC buffer_data;
    PFNGLBUFFERSUBDATAARBPROC buffer_subdata;
    PFNGLGETBUFFERSUBDATAARBPROC get_buffer_subdata;

    bool multitex;
    PFNGLACTIVETEXTUREARBPROC active_texture;
    PFNGLCLIENTACTIVETEXTUREARBPROC client_active_texture;
    PFNGLMULTITEXCOORD4FPROC multitex_coord;

    bool generate_mipmap;

    bool rescale_normal;

    boost::shared_ptr<WM> wm;

    inline Private (boost::shared_ptr<WM> wm) : wm(wm)
    {
      vbo =
      multitex =
      generate_mipmap =
      rescale_normal =
        false;
    }
  };

  GLExtensionProxy::GLExtensionProxy (boost::shared_ptr<WM> wm)
    : priv(new Private (wm))
  {
    if (!std::getenv ("SS_DISABLE_VBO"))
      priv->vbo = wm->has_gl_extension ("GL_ARB_vertex_buffer_object");

    if (priv->vbo)
    {
      priv->gen_buffers = (PFNGLGENBUFFERSARBPROC)
        wm->get_gl_extension_proc ("glGenBuffersARB");
      priv->delete_buffers = (PFNGLDELETEBUFFERSARBPROC)
        wm->get_gl_extension_proc ("glDeleteBuffersARB");
      priv->bind_buffer = (PFNGLBINDBUFFERARBPROC)
        wm->get_gl_extension_proc ("glBindBufferARB");
      priv->buffer_data = (PFNGLBUFFERDATAARBPROC)
        wm->get_gl_extension_proc ("glBufferDataARB");
      priv->buffer_subdata = (PFNGLBUFFERSUBDATAARBPROC)
        wm->get_gl_extension_proc ("glBufferSubDataARB");
      priv->get_buffer_subdata = (PFNGLGETBUFFERSUBDATAARBPROC)
        wm->get_gl_extension_proc ("glGetBufferSubDataARB");
    }

    if (!std::getenv ("SS_DISABLE_MULTITEX"))
      priv->multitex = wm->has_gl_extension ("GL_ARB_multitexture");

    if (priv->multitex)
    {
      priv->active_texture = (PFNGLACTIVETEXTUREARBPROC)
        wm->get_gl_extension_proc ("glActiveTextureARB");
      priv->client_active_texture = (PFNGLCLIENTACTIVETEXTUREARBPROC)
        wm->get_gl_extension_proc ("glClientActiveTextureARB");
      priv->multitex_coord = (PFNGLMULTITEXCOORD4FPROC)
        wm->get_gl_extension_proc ("glMultiTexCoord4fARB");
    }

    if (!std::getenv ("SS_DISABLE_GENERATE_MIPMAP"))
      priv->generate_mipmap = wm->has_gl_extension ("GL_SGIS_generate_mipmap");

    if (!std::getenv ("SS_DISABLE_RESCALE_NORMAL"))
      priv->rescale_normal = wm->has_gl_extension ("GL_EXT_rescale_normal");
  }

  bool
  GLExtensionProxy::has_vbo ()
  {
    return priv->vbo;
  }

  GLuint
  GLExtensionProxy::new_buffer ()
  {
    GLuint ret;

    if (!has_vbo ())
      throw std::logic_error ("Tried to use non-existent VBO support");

    priv->gen_buffers (1, &ret);
    
    return ret;
  }

  void
  GLExtensionProxy::delete_buffer (GLuint buffer)
  {
    if (!has_vbo ())
      throw std::logic_error ("Tried to use non-existent VBO support");

    return priv->delete_buffers (1, &buffer);
  }

  void
  GLExtensionProxy::bind_buffer (GLenum target, GLuint buffer)
  {
    if (!has_vbo ())
      throw std::logic_error ("Tried to use non-existent VBO support");

    return priv->bind_buffer (target, buffer);
  }

  void
  GLExtensionProxy::buffer_upload (GLenum target,
                                   GLsizeiptrARB size,
                                   const void *data,
                                   GLenum usage)
  {
    if (!has_vbo ())
      throw std::logic_error ("Tried to use non-existent VBO support");

    return priv->buffer_data (target, size, data, usage);
  }

  void
  GLExtensionProxy::buffer_update (GLenum target,
                                   GLintptrARB offset,
                                   GLsizeiptrARB size,
                                   const void *data)
  {
    if (!has_vbo ())
      throw std::logic_error ("Tried to use non-existent VBO support");

    return priv->buffer_subdata (target, offset, size, data);
  }

  void
  GLExtensionProxy::buffer_download (GLenum target,
                                     GLintptrARB offset,
                                     GLsizeiptrARB size,
                                     void *data)
  {
    if (!has_vbo ())
      throw std::logic_error ("Tried to use non-existent VBO support");

    return priv->get_buffer_subdata (target, offset, size, data);
  }

  bool
  GLExtensionProxy::has_multitex ()
  {
    return priv->multitex;
  }

  void
  GLExtensionProxy::activate_texture (GLenum tex)
  {
    if (!has_multitex ())
      throw std::logic_error ("Tried to use non-existent multitexture support");

    return priv->active_texture (tex);
  }

  void
  GLExtensionProxy::client_activate_texture (GLenum tex)
  {
    if (!has_multitex ())
      throw std::logic_error ("Tried to use non-existent multitexture support");

    return priv->client_active_texture (tex);
  }

  void
  GLExtensionProxy::multitex_coord (GLenum tex,
                                    GLfloat s,
                                    GLfloat t,
                                    GLfloat r,
                                    GLfloat q)
  {
    if (tex == GL_TEXTURE0_ARB)
      return glTexCoord4f (s, t, r, q);

    if (!has_multitex ())
      throw std::logic_error ("Tried to use non-existent multitexture support");

    return priv->multitex_coord (tex, s, t, r, q);
  }

  bool
  GLExtensionProxy::has_generate_mipmap ()
  {
    return priv->generate_mipmap;
  }

  bool
  GLExtensionProxy::has_rescale_normal ()
  {
    return priv->rescale_normal;
  }
}

