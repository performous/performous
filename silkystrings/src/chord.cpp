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

#include "chord.h"



Chord::Chord(int frt,double begin){
  this->fret=frt;
  this->start=begin;
  this->state=COMING;
  this->dt=0.075;
}
Chord::Chord(const Chord &c){
  this->fret=c.fret;
  this->event=c.event;
  this->start=c.start;
  this->end=c.end;
  this->dt=c.dt;
  this->state=c.state;
}

Chord::~Chord(){}
Chord &Chord::operator=(const Chord &c){
  if(&c!=this){
    this->fret=c.fret;
    this->event=c.event;
    this->start=c.start;
    this->end=c.end;
    this->dt=c.dt;
    this->state=c.state;
  }
  return *this;
}

unsigned char Chord::getEvent(){
  return this->event;
}

int Chord::getFret(){
  return this->fret;
}

void Chord::setEnd(double e){

  this->end=e;
}

bool Chord::isBetween(double time){

  return (time>this->start&&time<this->end);
}

Chord::ChordState Chord::getState(){
  
  return this->state;
}

void Chord::setState(Chord::ChordState s){
  
  this->state=s;
}

bool Chord::pressNow(double t){

  return (t>(this->start-this->dt)&&t<(this->start+dt));
}

double Chord::getDt(){

  return this->dt;
}



