#/bin/sh
# this script guess which is the vocals track of a song and name it "vocals.ogg"
# usage ./setvocals.sh [songdir]

cd "$1" || exit

function setvocals_rockband {
	if [ -f "song.ogg" -a -f "drums.ogg" -a -f "rhythm.ogg" -a -f "guitar.ogg" ];
	then mv song.ogg vocals.ogg;
	fi
}

function setvocals_guitarhero {
	if [ -f "song.ogg" -a -f "drumsFoFiX.ogg" -a -f "songFoFiX.ogg" -a -f "rhythm.ogg" -a -f "guitar.ogg" ];
	then mv songFoFiX.ogg vocals.ogg;
	fi
}

setvocals_rockband
setvocals_guitarhero
