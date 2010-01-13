mkdir build
cd build
cmake ../../ -DCMAKE_INSTALL_PREFIX=./Performous.app/Contents
make install

mkdir Performous.app/Contents/MacOS
mkdir Performous.app/Contents/Resources
mkdir Performous.app/Contents/Frameworks

mv Performous.app/Contents/bin/* Performous.app/Contents/MacOS/

cp ../resources/performous-launcher Performous.app/Contents/MacOS/
cp ../resources/performous.icns Performous.app/Contents/Resources
cp ../resources/Info.plist Performous.app/Contents/
cp -R ../resources/etc Performous.app/Contents/Resources

cp -R /Library/Frameworks/SDL.framework Performous.app/Contents/Frameworks/SDL.framework

dylibbundler -od -b -x ./Performous.app/Contents/MacOS/performous -d ./Performous.app/Contents/libs/
dylibbundler -of -b -x ./Performous.app/Contents/lib/performous/libda-1/libda_audio_dev_jack.so -d ./Performous.app/Contents/libs
dylibbundler -of -b -x ./Performous.app/Contents/lib/performous/libda-1/libda_audio_dev_pa19.so -d ./Performous.app/Contents/libs
dylibbundler -of -b -x ./Performous.app/Contents/lib/performous/libda-1/libda_audio_dev_tone.so -d ./Performous.app/Contents/libs

cd ..
