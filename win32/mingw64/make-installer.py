#!/usr/bin/env python3
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
import shutil

try:
    makensis_var = os.environ['MAKENSIS']
except KeyError:
    makensis_var = os.environ['makensis']
makensis = subprocess.Popen([makensis_var, '-INPUTCHARSET', 'UTF8', '-'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, encoding='utf-8')

if not os.path.isdir('dist'):
    os.mkdir('dist')
os.chdir('stage')

# Find the version number.
exepath = 'Performous.exe'
if 'WINDRES' in os.environ:
    custom_windres = os.environ['WINDRES']
else:
    custom_windres = 'windres'

for windres in [custom_windres, 'i686-w64-mingw32.shared-windres']:
    try:
        resources = subprocess.run([windres, exepath], stdout=subprocess.PIPE, encoding='utf-8')
    except:
        continue
    for line in resources.stdout.splitlines():
        if not line.strip().startswith('VALUE'):
            continue
        if 'ProductVersion' in line:
            version = line.strip().split('"')[-2]
            break
    else:
        version = 'unknown'
    break

if os.path.exists('~/.fonts.conf'):
	if os.path.isdir('~/.fonts.conf.d'):
		fcconfpath = ['~/.fonts.conf','~/.fonts.conf.d']
	elif os.path.isdir('/etc/fonts/conf.d'):
		fcconfpath = ['~/.fonts.conf','/etc/fonts/conf.d']
elif os.path.isdir('/opt/local/etc/fonts'):
	fcconfpath = ['/opt/local/etc/fonts/fonts.conf', '/opt/local/etc/fonts/conf.d']
elif os.path.isdir('/etc/fonts'):
	fcconfpath = ['/etc/fonts/fonts.conf', '/etc/fonts/conf.d']

dest = os.getcwd()
for path in fcconfpath:
	print ('Copying fontconfig configuration files from:', path)
	if not os.path.exists(os.path.join(dest,'etc')):
		os.mkdir(os.path.join(dest,'etc'))
	if os.path.isfile(path):
		shutil.copy(path,os.path.join(dest,'etc'))
	else:
		shutil.copytree(path,os.path.join(dest,'etc',os.path.basename(path)))
		
def instpath(*elements):
    return os.path.join(*elements).replace('/', '\\').replace('.\\', '')

makensis_script = fr'''!include "MUI2.nsh"
!include FileFunc.nsh

!define VERSION "{version!s}"
!define REGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Performous"

Name "Performous ${{VERSION}}"
OutFile "dist\Performous-${{VERSION}}.exe"
Unicode true

SetCompressor /SOLID lzma

ShowInstDetails show
ShowUninstDetails show

InstallDir "$PROGRAMFILES\Performous"
InstallDirRegKey HKLM ${{REGKEY}} "InstallLocation"

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
'''

for root, dirs, files in os.walk('.'):
    makensis_script += '  SetOutPath "$INSTDIR\\{}"\n'.format(instpath(root))
    for file in files:
        makensis_script += '  File "{}"\n'.format(instpath('stage', root, file))

makensis_script += r'''  WriteUninstaller "$INSTDIR\uninst.exe"
  SetShellVarContext all
  CreateDirectory "$INSTDIR\songs"
  CreateDirectory "$SMPROGRAMS\Performous"
  SetOutPath "$INSTDIR"
  CreateShortcut "$SMPROGRAMS\Performous\Performous.lnk" "$INSTDIR\Performous.exe"
  CreateShortCut "$SMPROGRAMS\Performous\ConfigureSongDirectory.lnk" "$INSTDIR\ConfigureSongDirectory.bat"
  CreateShortCut "$SMPROGRAMS\Performous\Songs.lnk" "$INSTDIR\songs"
  CreateShortcut "$SMPROGRAMS\Performous\Uninstall.lnk" "$INSTDIR\uninst.exe"
  WriteRegStr HKLM ${REGKEY} "DisplayName" "Performous"
  WriteRegStr HKLM ${REGKEY} "UninstallString" "$\"$INSTDIR\uninst.exe$\""
  WriteRegStr HKLM ${REGKEY} "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM ${REGKEY} "DisplayIcon" "$INSTDIR\Performous.exe"
  WriteRegStr HKLM ${REGKEY} "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM ${REGKEY} "HelpLink" "http://performous.org/"
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM ${REGKEY} "EstimatedSize" "$0"
  WriteRegDWORD HKLM ${REGKEY} "NoModify" 1
  WriteRegDWORD HKLM ${REGKEY} "NoRepair" 1
SectionEnd

Section Uninstall
  RmDir "$INSTDIR\songs"
'''

for root, dirs, files in os.walk('.', topdown=False):
    for dir in dirs:
        makensis_script += '  RmDir "$INSTDIR\\{}"\n'.format(instpath(root, dir))
    for file in files:
        makensis_script += '  Delete "$INSTDIR\\{}"\n'.format(instpath(root, file))
    makensis_script += '  RmDir "$INSTDIR\\{}"\n'.format(instpath(root))

makensis_script += r'''  Delete "$INSTDIR\uninst.exe"
  RmDir "$INSTDIR"
  SetShellVarContext all
  Delete "$SMPROGRAMS\Performous\Performous.lnk"
  Delete "$SMPROGRAMS\Performous\ConfigureSongDirectory.lnk"
  Delete "$SMPROGRAMS\Performous\Songs.lnk"
  Delete "$SMPROGRAMS\Performous\Uninstall.lnk"
  RmDir "$SMPROGRAMS\Performous"
  DeleteRegKey HKLM ${REGKEY}
SectionEnd
'''

print(makensis.communicate(input=makensis_script)[0], file=sys.stdout)
if makensis.returncode != 0:
    print ('Installer compilation failed.', file=sys.stderr)
    sys.exit(1)
else:
    print ('\ndist/Performous-{}.exe is ready.'.format(version))
