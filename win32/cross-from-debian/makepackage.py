#!/usr/bin/env python
# NSIS script generator for Performous.
# Copyright (C) 2010 John Stumpo
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import subprocess
import sys

try:
    makensis = subprocess.Popen([os.environ['MAKENSIS'], '-'], stdin=subprocess.PIPE)
except KeyError:
    makensis = subprocess.Popen(['makensis', '-'], stdin=subprocess.PIPE)

if not os.path.isdir('dist'):
    os.mkdir('dist')
os.chdir('stage')

# Find the version number.
try:
    resources = subprocess.Popen([os.environ['WINDRES'], 'bin/performous.exe'], stdout=subprocess.PIPE)
except:
    try:
        resources = subprocess.Popen(['windres', 'bin/performous.exe'], stdout=subprocess.PIPE)
    except:
        resources = subprocess.Popen(['i586-mingw32msvc-windres', 'bin/performous.exe'], stdout=subprocess.PIPE)
for line in resources.stdout.readlines():
    if not line.strip().startswith('VALUE'):
        continue
    if 'ProductVersion' in line:
        version = line.strip().split('"')[-2]
        break
else:
    version = 'unknown'


makensis.stdin.write(r'''!include "MUI2.nsh"

!define VERSION "%s"

Name "Performous ${VERSION}"
OutFile "dist\Performous-${VERSION}-win32.exe"

SetCompressor /SOLID lzma

ShowInstDetails show
ShowUninstDetails show

InstallDir "$PROGRAMFILES\Performous"
InstallDirRegKey HKLM "Software\Performous" ""

RequestExecutionLevel admin

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section
''' % version)

for root, dirs, files in os.walk('.'):
    makensis.stdin.write('  SetOutPath "$INSTDIR\\%s"\n' % root.replace('/', '\\'))
    for file in files:
        makensis.stdin.write('  File "%s"\n' % os.path.join('stage', root, file).replace('/', '\\'))

makensis.stdin.write(r'''  WriteRegStr HKLM "Software\Performous" "" "$INSTDIR"
  WriteUninstaller "$INSTDIR\uninst.exe"
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\Performous"
  CreateShortcut "$SMPROGRAMS\Performous\Performous.lnk" "$INSTDIR\bin\performous.exe"
  CreateShortcut "$SMPROGRAMS\Performous\Uninstall.lnk" "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Performous" "DisplayName" "Performous"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Performous" "UninstallString" "$\"$INSTDIR\uninst.exe$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Performous" "DisplayIcon" "$INSTDIR\bin\performous.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Performous" "DisplayVersion" "${VERSION}"
SectionEnd

Section Uninstall
''')

for root, dirs, files in os.walk('.', topdown=False):
    for dir in dirs:
        makensis.stdin.write('  RmDir "$INSTDIR\\%s"\n' % os.path.join(root, dir).replace('/', '\\'))
    for file in files:
        makensis.stdin.write('  Delete "$INSTDIR\\%s"\n' % os.path.join(root, file).replace('/', '\\'))
    makensis.stdin.write('  RmDir "$INSTDIR\\%s"\n' % root.replace('/', '\\'))

makensis.stdin.write(r'''  Delete "$INSTDIR\uninst.exe"
  RmDir "$INSTDIR"
  SetShellVarContext all
  Delete "$SMPROGRAMS\Performous\Performous.lnk"
  Delete "$SMPROGRAMS\Performous\Uninstall.lnk"
  RmDir "$SMPROGRAMS\Performous"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Performous"
  DeleteRegKey /ifempty HKLM "Software\Performous"
SectionEnd
''')

makensis.stdin.close()
if makensis.wait() != 0:
    print >>sys.stderr, 'Installer compilation failed.'
    sys.exit(1)
else:
    print '\ndist/Performous-%s-win32.exe is ready.' % version
