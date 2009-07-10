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

#include "Action.h"
#include "Sound.h"
#include <stdexcept>
#include <iomanip>

#include <boost/lexical_cast.hpp>

namespace SilkyStrings {

	namespace { bool chordlt(Chord const& a, Chord const& b) { return a.getStart() < b.getStart(); } }

    std::vector<double> deltatimes;
	
	Action::Action(boost::shared_ptr<WM> w,
		boost::shared_ptr<Input> in,
		MidiFileParser &p,
		boost::shared_ptr<GameView> v,
		boost::shared_ptr<Reaction> reaction,
		boost::shared_ptr<Sound> sound,
		std::string songPath):
	  difficulty(), wm(w), input(in), reaction(reaction), sound(sound), view(v), track(0), trackcount(0)
	{
		files.push_back(songPath + "song.ogg");
		files.push_back(songPath + "guitar.ogg");
		files.push_back(songPath + "rhythm.ogg");
		parser=&p;
		statetime=0;
		gamerunning=true;
		for (int i=0; i<5; i++) pickwaspressed[i]=false;
		// Count tracks for multi-track songs (instrument track names begin with "PART ")
		for (size_t i = 0; i < parser->tracks.size(); ++i) {
			if (parser->tracks[i].name.substr(0, 5) != "PART ") break;
			++trackcount;
		}
		if (trackcount == 0) trackcount = 1; // Single-track song
		if (trackcount + 1 > files.size()) throw std::runtime_error("Too many tracks in song");
	}
	
	Action::~Action() {}

	bool Action::set_difficulty(Difficulty level) {
		Chords c;
		uint8_t basepitch;
		switch (level) {
			case DIFFICULTY_SUPAEASY: basepitch = 0x3c; break;
			case DIFFICULTY_EASY: basepitch = 0x48; break;
			case DIFFICULTY_MEDIUM: basepitch = 0x54; break;
			case DIFFICULTY_AMAZING: basepitch = 0x60; break;
			default: throw std::logic_error("Invalid difficulty level");
		}
		for (int fret = 1; fret <= 5; ++fret) {
			std::map<MidiFileParser::Pitch, std::vector<MidiFileParser::Note> >::iterator p = parser->tracks[track].notes.find(basepitch + fret - 1);
			if (p == parser->tracks[track].notes.end()) continue;
			for (std::vector<MidiFileParser::Note>::const_iterator it = p->second.begin(); it != p->second.end(); ++it) {
				c.push_back(Chord(fret, parser->get_seconds(it->begin)));
				c.back().setEnd(parser->get_seconds(it->end));
			}
		}
		if (c.empty()) return false;
		std::sort(c.begin(), c.end(), chordlt);
		difficulty = level;
		chords = c;

        std::vector<double> new_beats;
        double time = 0;
        int i = 0;
        while(time < chords.back().getEnd()) {
            time = parser->get_seconds(i*parser->division);
            new_beats.push_back(time);
            i++;
        }
        beats = new_beats;

		return true;
	}

	void Action::display_level() {
		static char const* difficultystr[] = { "Supaeasy", "Easy", "Medium", "Amazing" };
		std::string s(difficultystr[difficulty]);
		if (trackcount > 1) s += "/" + track_name();
		view->set_static_notification(s);
	}

	void Action::autoselect_difficulty() {
		// First try keeping the current level
		if (set_difficulty(difficulty)) return;
		// Choose the easiest available difficulty level
		Difficulty d = Difficulty();
		while (d == difficulty || !set_difficulty(d)) {
			d = Difficulty(d + 1);
			if (d == DIFFICULTYCOUNT) throw std::runtime_error("Notes not found");
		}
	}
	
	void Action::switch_track() {
		track = (track + 1) % trackcount;
		autoselect_difficulty();
	}
	
	std::string Action::track_name() {
		std::string name(parser->tracks[track].name);
		if (name == "T1 GEMS") return "default";
		if (name.substr(0, 5) == "PART ") return name.substr(5);
		return name;
	}
	
	void Action::start() {
		autoselect_difficulty();
		for (size_t i = 0; i <= trackcount; ++i) {
            std::cout << "Preloading " << files.at(i) << std::endl;
            sound->preload(files[i]);
        }

		int seconds = -5;
		int countdown = -4;
		double initialtime = wm->get_clock() - seconds;
		statetime = 0.0;
		bool oldpick = false;

		while (gamerunning) {
			if (wm->update_beginning_of_frame()) break;
			double dt = wm->get_clock() - initialtime - statetime;
			statetime = wm->get_clock() - initialtime - offset;
			view->update_beginning_of_frame(statetime);

			if (wm->get_clock() - initialtime > seconds) {
				if (seconds == -5) { view->set_popup_notification("PICK LEVEL", 1.0); display_level(); }
				if (seconds > countdown && seconds < 0) {
					countdown = seconds;
					view->set_popup_notification(boost::lexical_cast<std::string>(-seconds), 1.0);
				}
				if (seconds == 0) {
					for (size_t i = 0; i <= trackcount; ++i) {
						Sound::Handle s = sound->play(files[i]);
						if (i == track + 1) reaction->registerGuitarTrack(s);
					}
				}
				++seconds;
			}

			if (seconds < 0) {
				bool pick = input->pick_pressed();
				if (!oldpick && pick) {
					int fret = -1;
					for (int i = 0; i < 5; i++) if (input->fret_pressed(i + 1)) fret = (fret == -1) ? i : -2;
					if (fret >= 0 && fret <= 3 && set_difficulty(static_cast<Difficulty>(fret))) display_level();
					else if (fret == 4) { switch_track(); display_level(); }
					seconds = -4;
					initialtime = wm->get_clock() - seconds;
				}
				oldpick = pick;
			} else {
				clearUsedChords();
				for (int i=1;i<6;i++) {
					if (input->fret_pressed(i)) {
						if (input->pick_pressed()) {
							if (!pickwaspressed[i]) { // Player picked on this fret
								pickwaspressed[i]=true;
								checkFretPressed(i);
							} else {  // Player had the pick pressed, but didn't pick
								if (fretHeld(i)) reaction->correctHold(dt);
							}
						} else {  // Player kept the fret pressed
							pickwaspressed[i]=false;
							if (fretHeld(i)) reaction->correctHold(dt); // There might be a long chord
							checkFretFailed(i); // And there might be a missed chord
						}
					} else if (!input->pick_pressed()) { // Nothing was pressed
						pickwaspressed[i]=false;
						checkFretFailed(i);
					} else { // Pick was pressed while no fret was
						checkFretFailed(i);
					}
				}
			}
			if (input->quit_pressed()) {
                showStatistics();
                break;
            }
			sound->update();
			drawGameView();
			for (int i = 0; i < 5; i++) view->draw_fret(i + 1, input->fret_pressed (i + 1));
			view->update_end_of_frame();
			wm->update_end_of_frame();
		}
	}

    void Action::showStatistics(){
        double average = 0;
        for (unsigned i=0; i<deltatimes.size(); i++) {
            average += deltatimes[i];
        }
        average = average / deltatimes.size();

        double variance = 0;
        for (unsigned i=0; i<deltatimes.size(); i++) {
            variance += (average - deltatimes[i]) * (average - deltatimes[i]);
        }
        variance = variance / deltatimes.size();

        std::cout << "Average: " << std::setprecision(6) << average << " Variance: " << variance << std::endl;

    }

	void Action::drawGameView() {
		if (beats.size() > 0) {
			std::vector<double>::iterator it = beats.begin();
			while (*it < (statetime+10)) {
				view->draw_beat(*it);
				it++;
				if (it == beats.end()) break;
			}
        }
		if (chords.size()>0) {
			iter it=chords.begin();
			while (it->getStart()<(statetime+10)) {
				GameView::ChordViewState state;
				if (it->getState()==Chord::COMING) state=GameView::CHORD_VIEW_STATE_PASSIVE;
				else if (it->getState()==Chord::HELD) state=GameView::CHORD_VIEW_STATE_HELD_DOWN;
				else state=GameView::CHORD_VIEW_STATE_FAILED;
				view->draw_chord(it->getFret(),it->getStart(),it->getEnd(),state);
				it++;
				if (it==chords.end()) break;
			}
		}
	}

	void Action::clearUsedChords() {
		if (chords.size()>0) {
			iter it=chords.begin();
			if (it->getEnd()<(statetime-0.8)) {
				while (it->getEnd()<(statetime-0.3)) {
					it++;
					if (it==chords.end()) break;
				}
				chords.erase(chords.begin(),it);
			}
		}
		if (beats.size()>0) {
			std::vector<double>::iterator  it = beats.begin();
			if (*it < statetime-0.8) {
				while (*it < statetime-0.3) {
					it++;
					if (it==beats.end()) break;
				}
				beats.erase(beats.begin(),it);
			}
		}
	}

	bool Action::checkFretPressed(int f) {
		if (chords.size()>0) {

            Chord* best_chord = findClosestChord(f);
            if(best_chord == NULL) { // No chord on this fret
                std::cout << "Wrong: No chord on this fret... at all" << std::endl;
                reaction->playWrong();
                return false;
            }
			double dt = statetime - best_chord->getStart();
            if (SYNCHING) {
                std::cout << dt << std::endl;
            }
            deltatimes.push_back(dt);
		    if (best_chord->pressNow(statetime)) {
				if (best_chord->getState()==Chord::COMING) {
					best_chord->setState(Chord::HELD);
					reaction->correctPick(dt);
				} else {
					best_chord->setState(Chord::HELD);
				}
            } else {
                std::cout << "Wrong: No chord on this fret is close enough" << std::endl;
                reaction->playWrong();
                return false;
            }
		}
		return true;
	}

    Chord* Action::findClosestChord(int f) {
        Chord* best_chord = NULL;
		iter it = chords.begin();
		double dt = statetime;
		if (dt<0) dt *= -1;
        double old_dt = dt +1;
        while (it!=chords.end()) {
            if(it->getFret() == f) {
                old_dt = dt;
                dt = statetime - it->getStart();
			    if (dt<0) dt *= -1;
                if (dt > old_dt) {
                    return best_chord;
                } else {
                    best_chord = &(*it);
                }
            }
            it++;
        }
        return NULL;
    }

	
	bool Action::fretHeld(int f) {
		if (chords.size()>0) {
			iter it=chords.begin();
			bool isfound=false;
			while (it->getStart()<(statetime+it->getDt())) {
				if ((it->getFret()==f&&it->getState()==Chord::HELD)&&(it->isBetween(statetime)||it->pressNow(statetime))) {
					isfound=true;
					break;
				} else {
					it++;
					if (it==chords.end()) break;
				}
			}
			return isfound;
		}
		return false;
	}

	void Action::checkFretFailed(int f) {
		if (chords.size()>0) {
			iter it=chords.begin();
			while (it->getStart()<(statetime+it->getDt())) {
				if (it->getFret()==f && it->getEnd()+it->getDt() < statetime && it->getState() == Chord::COMING) {
                    std::cout << "Wrong: You missed a chord" << std::endl;
					it->setState(Chord::FAILED);
					reaction->playWrong(false);
					break;
				} else if (it->getFret() == f && it->isBetween(statetime+0.3) && it->getState() == Chord::HELD && !input->fret_pressed(f)) { //XXX: Harcoded value
                    std::cout << "Wrong: You didn't hold that chord long enough" << std::endl;
					it->setState(Chord::FAILED);
					reaction->playWrong(false);
					break;
				} else {
					it++;
					if (it==chords.end()) break;
				}
			}
		}
	}

	void Action::addReaction(boost::shared_ptr<Reaction> r) {
		reaction=r;
	}
}
