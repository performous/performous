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

#include "midiEvent.h"

MidiEvent::MidiEvent(int frt,unsigned char evt, double tim):fret(frt),event(evt),time(tim){}
MidiEvent::MidiEvent(const MidiEvent &e){
  this->fret=e.fret;
  this->event=e.event;
  this->time=e.time;
}
MidiEvent::~MidiEvent(){}
MidiEvent &MidiEvent::operator=(const MidiEvent &e){

  if(&e!=this){
    this->fret=e.fret;
    this->event=e.event;
    this->time=e.time;
  }

  return *this;
}


unsigned char MidiEvent::getEvent(){

  return this->event;
}

int MidiEvent::getFret(){

  return this->fret;
}

double MidiEvent::getTime(){

  return this->time;
}

