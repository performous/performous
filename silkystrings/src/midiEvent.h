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


/**
 * The MidiEvent class that holds information for single midi-event
 */

class MidiEvent{
  
 public:
  
  /** Constructor
   * 
   *Constructs new MidiEvent
   *
   * @param fret Fret(1,2,3,4,5) in which the event is connected.
   * @param evt Tells the type of midievent Noteon or Noteoff.
   * @param time Time, absolute time in seconds. Tells when the event should happen.
   */
  
  MidiEvent(int fret,unsigned char evt, double time);
  MidiEvent(const MidiEvent &e);
  ~MidiEvent();
  MidiEvent &operator=(const MidiEvent &e);
  
  /** Tells which type the midievent is.
   *
   * @return Midievent type.
   */

  unsigned char getEvent();

  /** Tells the quitar fret where the event is linked.
   *
   * @return Quitar fret.
   */

  int getFret();

  /** Tells when the midievent happens
   *
   * @ return the happening time in seconds.
   */

  double getTime();

private:

  int fret;
  unsigned char event;
  double time;
};
