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

#ifndef __SS_TEXTRENDERER_H_
#define __SS_TEXTRENDERER_H_

#include <string>

#include <boost/shared_ptr.hpp>

namespace SilkyStrings
{
  struct GLExtensionProxy;
  
  /** Advanced text renderer.
   *
   * The TextRenderer class provides advanced text rendering capabilities. It
   * can render numerous formats of outline and bitmap fonts.
   */

  class TextRenderer
  {
    public:

      /** Constructor.
       *
       * @param font Path to a font file to use.
       * @param height Vertical pixel resolution of the rendered text.
       * @param proxy A valid GLExtensionProxy instance.
       */

      TextRenderer (const std::string &font,
                    unsigned height,
                    boost::shared_ptr<GLExtensionProxy> proxy);

      /** Render and cache text.
       *
       * Renders text to the current GL context. The text is rendered centered
       * and its vertical coordinates will span the range [-0.5, 0.5]. The
       * horizontal coordinate range depends on the font and the text to render.
       *
       * Strings of text rendered with this method will be cached for faster
       * rendering in future frames.
       *
       * @param text Text to render.
       */

      void
      render (const std::string &text);

      /** Discard text from cache.
       *
       * Discards text previously cached by render() from the cache, freeing up
       * resources.
       *
       * Once you know you won't be drawing a certain string of text in a while,
       * you should discard it from the cache to free memory.
       *
       * @param text Text to discard from the cache.
       */

      void
      discard (const std::string &text);

    private:

      void
      populate (const std::string &text);

      struct Private;
      boost::shared_ptr<Private> priv;
  };
}

#endif

