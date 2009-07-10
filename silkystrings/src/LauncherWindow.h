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

#ifndef LAUNCHERWINDOW_H
#define LAUNCHERWINDOW_H

#include <QApplication>
#include <QDialog>
#include <QComboBox>
#include <QListWidget>
#include <QCheckBox>

namespace Launcher {
  

  /**
   * LauncherWindow is launcher's main window (there is no other)
   */
  class LauncherWindow : public QDialog {
		Q_OBJECT

	public:

    /** Constructor
     * Setup a new launcher window and search for songs in ../resources/songs directory
     */
    LauncherWindow();
		
	private slots:
    void play();
    void loadSongs();


	private:
    bool checkValidity();


		QComboBox *gameButtons[7];
		QListWidget *songList;
		QComboBox *resolution;
    QCheckBox *fullscreen;
    QComboBox *difficulty;
	};
}

#endif

