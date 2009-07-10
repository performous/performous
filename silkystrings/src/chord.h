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

/** The Chord class that contains information for single chord.
 *
 */

#ifndef __SS_CHORD_H_
#define __SS_CHORD_H_

class Chord{

  
 public:

  static const double CHORD_POOR = 0.075;
  static const double CHORD_GOOD = 0.050;
  static const double CHORD_GREAT = 0.020;
  static const double CHORD_PERFECT = 0.010;

  /**
   * Tells in which state the chord is.
   */

  enum ChordState{
    
    COMING,
    HELD,
    FAILED
  };

  /** Constructor.
   *
   * @param fret The quitar fret where the chord songs.
   * @param begin The start time of the chord.
   */

  Chord(int fret,double begin);
  Chord(const Chord &c);
  ~Chord();
  Chord &operator=(const Chord &c);
  unsigned char getEvent();

  /** Tells the quitar fret where the chord songs.
   *
   * @return quitar fret.
   */

  int getFret();

  /** Tells how long the chord last.
   *
   * @return Time between end and begin.
   */

  double getTime() const { return end - start; }

  /** Tells when the chord should be played.
   *
   * @return Chords start time in seconds.
   */

  double getStart() const { return start; }

  /** Tells when the chord ends.
   *
   * @return Chords end in seconds.
   */

  double getEnd() const { return end; }

  /** Sets the end time of chord in seconds.
   *
   * @param end End time in seconds.
   */

  void setEnd(double end);

  /** Tells if the given time is between begin and end.
   *
   * @param time Time is between begin or end or is not.
   * @return true if time is between false otherwise.
   */

  bool isBetween(double time);

  /** Tells if the player should pick this chord at given time
   *
   * @param time Time when the player tries to pick this chord.
   * @return true if chord should be picked at given time false otherwise.
   */

  bool pressNow(double time);

  /** Tells how long is the pick time.
   *
   * @return Size of picktime in seconds.
   */

  double getDt();

  /** Tells in which state the chord is.
   *
   * @return Chord-state.
   */

  ChordState getState();

  /** Set the chord state
   *
   * @param state Chord-state where the chord-changes.
   */

  void setState(ChordState state);

private:

 
  int fret;
  unsigned char event;
  double start;
  double end;
  ChordState state;
  double dt;
};

#endif
