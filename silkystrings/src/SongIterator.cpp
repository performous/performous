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

#include "SongIterator.h"

#include <string>


Launcher::SongIterator::SongIterator(boost::filesystem::path path): iter(path) {
	while(iter != boost::filesystem::recursive_directory_iterator() && !checkValidity()) ++iter;
}

Launcher::SongIterator::SongIterator() {}


bool Launcher::SongIterator::operator!=(const SongIterator &iter){
	return this->iter != iter.iter;
}

bool Launcher::SongIterator::operator==(const SongIterator &iter){
	return this->iter == iter.iter;
}

std::string Launcher::SongIterator::operator*(){
	return iter->leaf();
}

Launcher::SongIterator Launcher::SongIterator::operator++(){
	++iter;
	while(iter != boost::filesystem::recursive_directory_iterator() && !checkValidity()) ++iter;
	return *this;
}

bool Launcher::SongIterator::checkValidity(){
	boost::filesystem::path p = *iter;
	return  exists(p/"guitar.ogg") &&
			exists(p/"notes.mid") &&
			exists(p/"song.ini") &&
			exists(p/"song.ogg");
}
