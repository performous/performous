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

#ifndef SONGITERATOR_H
#define SONGITERATOR_H

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <string>


namespace Launcher{


  /**
   * Iterates through given path and searches for valid song directories
   */
	class SongIterator{
	public:
		
    /** Constructor
     * @param path path to iterate
     */
    SongIterator(boost::filesystem::path path);
		
    /** Default constructor
     * Creates an end-of-iteration iterator
     */
    SongIterator();

    /** operator !=
     * @param iter iterator to compare with
     */
		bool operator!=(const SongIterator &iter);

    /** operator ==
     * @param iter iterator to compare with
     */
    bool operator==(const SongIterator &iter);
	
    /** operator *
     * @return current path name
     */
    std::string operator*();

    /** operator++ (prefix)
     * @return this
     */
		Launcher::SongIterator operator++();

	private:
		bool checkValidity();

		boost::filesystem::directory_iterator iter;
	};
}

#endif

