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

#include "Sound.h"
#include "Util.h"

#include <stdexcept>
#include <map>
#include <utility>
#include <set>

#include <SDL.h>
#include <SDL_mixer.h>

namespace SilkyStrings
{
  const int NUM_CHANNELS = 32;

  namespace
  {
    class SDLMixerLibrary
    {
      public:

        SDLMixerLibrary ()
        {
          if (!refcount++)
          {
            if (SDL_Init (SDL_INIT_AUDIO))
              throw std::runtime_error (
                  std::string ("Failed to initialize SDL: ") + SDL_GetError ());

            if (Mix_OpenAudio (44100, AUDIO_S16LSB, 2, 1536))
              if (Mix_OpenAudio (44100, AUDIO_S16MSB, 2, 1536))
                throw std::runtime_error ("Failed to open audio device");

            Mix_AllocateChannels (NUM_CHANNELS);
          }
        }

        ~SDLMixerLibrary ()
        {
          if (!--refcount)
          {
            Mix_CloseAudio ();
            SDL_Quit ();
          }
        }

      private:

        static unsigned refcount;
    };

    class Sample
    {
      public:

        Sample (const ::std::string &file)
        {
          sample = Mix_LoadWAV (file.c_str ());

          if (!sample)
            throw std::runtime_error ("Failed to load sample " + file + ": "
                + Mix_GetError ());
        }

        ~Sample ()
        {
          Mix_FreeChunk (sample);
        }

        operator Mix_Chunk * ()
        {
          return sample;
        }

      private:

        Mix_Chunk *sample;
    };

    unsigned SDLMixerLibrary::refcount = 0;
  }

  struct Sound::Private
  {
    SDLMixerLibrary lib;
    std::map<std::string, boost::shared_ptr<Sample> > samples;
    std::map<Handle, int> playing_channels;
    Handle serial;

    static Private *instance;

    Private ()
      : serial (0)
    {
      if (instance)
        throw std::logic_error ("Only one instance allowed at a time!");

      instance = this;
    }

    ~Private ()
    {
      /* guarantee proper cleanup ordering */

      for (std::map<Handle, int>::iterator i = playing_channels.begin ();
           i != playing_channels.end (); ++i)
        Mix_HaltChannel (i->second);

      samples.clear ();

      instance = NULL;
    }
  };

  Sound::Private *
  Sound::Private::instance = NULL;

  Sound::Sound ()
    : priv (new Private ())
  {
  }

  void
  Sound::preload (const ::std::string &filename)
  {
    if (!priv->samples[filename])
      priv->samples[filename] = boost::shared_ptr<Sample> (new Sample (filename));
  }

  Sound::Handle
  Sound::play (const ::std::string &filename)
  {
    if (priv->samples.find (filename) == priv->samples.end ())
      preload (filename);

    int channel = Mix_PlayChannel (-1, *(priv->samples[filename]), 0);

    if (channel == -1)
    {
      SS_WARN ("failed to allocate channel for sound %s", filename.c_str ());
      return 0;
    }

    priv->playing_channels[++priv->serial] = channel;

    return priv->serial;
  }

  void
  Sound::set_volume (Handle to_mute, double volume)
  {
    if (volume < 0 || volume > 1)
      throw std::out_of_range ("Volume must be between 0 and 1");

    if (priv->playing_channels.find (to_mute) == priv->playing_channels.end ())
    {
      SS_WARN ("Sound %u not found, probably expired already", to_mute);
      return;
    }

    Mix_Volume (priv->playing_channels[to_mute], int(MIX_MAX_VOLUME * volume));
  }

  void
  Sound::update ()
  {
    std::set<Handle> to_erase;

    for (std::map<Handle, int>::iterator i = priv->playing_channels.begin ();
          i != priv->playing_channels.end (); i++)
    {
      if (!Mix_Playing (i->second))
        to_erase.insert (i->first);
    }

    for (std::set<Handle>::iterator i = to_erase.begin ();
        i != to_erase.end (); ++i)
    {
      priv->playing_channels.erase (*i);
    }
  }
}

