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

#ifndef __SS_UTIL_H__
#define __SS_UTIL_H__

#include <stdio.h>

#define SS_WARN(format, ...) \
  {fprintf (stderr, "WARNING: " __FILE__ ":%u: " format "\n", __LINE__, ##__VA_ARGS__);}

#define SS_FONT \
  ("resources/Share-TechMono.ttf")

#endif
