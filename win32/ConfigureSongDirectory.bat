@echo off
set /p songdir=Enter the path to your song dir: 

echo Performous will now launch.
echo Go to the song browser and verify you typed the path correctly.
echo Then go to configuration menu and hit Ctrl+S to save the setting.
pause

performous.exe %songdir%
