#!/bin/bash
#
# this is revision 2
#

[ -z "$USDB_DEST_DIR" ] && USDB_DEST_DIR="${HOME}/.ultrastar/songs"
[ ! -e "$USDB_DEST_DIR" ] && mkdir -p "$USDB_DEST_DIR"

set -eu

if [ $# != 1 ] ; then
  echo " >>> This script should help you to import .txt files into your"
  echo " >>> song repository or update .txt files currently in the repository"
  echo " >>> by adding a video, cover or background image."
  echo " >>> It will guide you to youtube/google image search, insert the"
  echo " >>> proper tags and copy everything to your USDB_DEST_DIR."
  echo " >>> "
  echo " >>> usage: "`basename $0`" [songfile.txt]"
  echo " >>> "
  exit
fi

if [ ! -e "$1" ] ; then
  echo " >>> error: file \"$1\" does not exist."
  exit 1
fi

# testing dependencies
if which wget > /dev/null 2>/dev/null ; then
  DOWNLOAD_CMD="wget -O"
elif which curl > /dev/null 2>/dev/null ; then
  DOWNLOAD_CMD="curl -o"
else
  echo " >>> error: install either wget or curl."
  exit 1
fi
if ! which youtube-dl > /dev/null 2>/dev/null ; then
  echo " >>> error: youtube-dl not found in the path."
  echo " >>> get ih here: http://www.arrakis.es/~rggi3/youtube-dl/"
  exit 1
fi

function escape {
  perl -MCGI -e \ 'print CGI->escape(@ARGV), "\n"' "$1"
}

echo " >>> USDB_DEST_DIR = $USDB_DEST_DIR"

WORK_DIR=`dirname "$1"`

cd "$WORK_DIR"
TXT=`basename "$1"`

# remove backslashes from the filename (beta)
if echo "$TXT" | grep -q \\\\ ; then
  NEWTXT=`echo "$TXT" | sed -e s:\\\\\\\\::g`
  echo " >>> removing backslash from filename..."
  mv "$TXT" "$NEWTXT"
  TXT="$NEWTXT"
fi

PREFIX=`basename "$TXT" ".txt"`
TMP="${PREFIX}.old"

# convert \r\n --> \n
if grep -q $'\r' "$TXT" ; then
  echo " >>> converting from CRLF to LF"
  mv "$TXT" "$TMP"
  sed -e s:\\\r::g -- "$TMP" > "$TXT"
  rm "$TMP"
fi

while file "$TXT" | grep -q ISO ; do
  echo
  echo " >>> the files is not UTF-8, it will be opened in gedit for inspection."
  echo " >>> Save it as UTF-8, press return to continue, type 'no' to cancel."
  echo -n " >>> any key ... "
  read REPLY
  if [ "x$REPLY" = "xno" ] ; then
    break
  else
    gedit "$TXT"
    [ -e "${TXT}~" ] && rm "${TXT}~"
  fi
done

ARTIST=`grep     "#ARTIST:"     "$TXT" | cut -d : -f 2`
TITLE=`grep      "#TITLE:"      "$TXT" | cut -d : -f 2`
MP3=`grep        "#MP3:"        "$TXT" | cut -d : -f 2`
BPM=`grep        "#BPM:"        "$TXT" | cut -d : -f 2`
VIDEO=`grep      "#VIDEO:"      "$TXT" | cut -d : -f 2`
COVER=`grep      "#COVER:"      "$TXT" | cut -d : -f 2`
BACKGROUND=`grep "#BACKGROUND:" "$TXT" | cut -d : -f 2`

if [ "x$ARTIST" = "x" -o "x$TITLE" = "x" -o "x$BPM" = "x" ] ; then
  echo " >>> error: this does not seem to ba a valid ultrastar file."
  exit 1
fi

if test -z "$MP3" || test ! -e "$MP3" ; then
  GET_VIDEO=yes
elif test -z "$VIDEO" || test ! -e "$VIDEO" ; then
  echo
  echo " >>> There is an MP3 that looks good, but no video."
  echo " >>> If you want to delete the .mp3 and look for a video, enter yes."
  echo -n " >>> delete the MP3? [no]: "
  read DEL
  if [ "x$DEL" = "xyes" ] ; then
    rm "$MP3"
    GET_VIDEO=yes
  else
    GET_VIDEO=no
  fi
else
  GET_VIDEO=no
fi

# VIDEO not set/does not exist
if [ $GET_VIDEO = yes ] ; then

  echo

  # remove MP3 VIDEO and VIDEOGAP
  echo " >>> removing old #MP3 #VIDEO and #VIDEOGAP tags"
  mv "$TXT" "$TMP"
  grep -v "#MP3:" "$TMP" | grep -v "#VIDEO:" | grep -v "#VIDEOGAP:" > "$TXT"
  rm "$TMP"

  # download video
  ESCAPED=`escape "\"$ARTIST\" \"$TITLE\""`
  URL="http://www.youtube.com/results?search_query=$ESCAPED"
  echo " >>> Please look for the song here, then enter the desired video url below"
  echo " >>> $URL"
  echo -n " >>> youtube url: "
  read YOUTUBE_URL
  [ -z "$YOUTUBE_URL" ] && echo " >>> canceled." && exit
  MP3="${PREFIX}.mp4"
  VIDEO="${PREFIX}.mp4"
  echo " >>> running youtube-dl ..."
  youtube-dl -b -o "$VIDEO" "$YOUTUBE_URL"
  
  echo " >>> adding new #MP3 and #VIDEO tags"
  mv "$TXT" "$TMP"
  echo "#MP3:${MP3}" > "$TXT"
  echo "#VIDEO:${VIDEO}" >> "$TXT"
  cat "$TMP" >> "$TXT"
  rm "$TMP"
  
fi

# COVER does not exist
if test -z "$COVER" || test ! -e "$COVER" ; then

  echo

  # remove COVER
  if [ -n "$COVER" ] ; then
    echo " >>> removing old #COVER tag"
    mv "$TXT" "$TMP"
    grep -v "#COVER:" "$TMP" > "$TXT"
    rm "$TMP"
  fi

  # download image
  ESCAPED=`escape "\"$ARTIST\" \"$TITLE\" cover"`
  URL="http://images.google.com/images?q=$ESCAPED"
  echo " >>> Please look for the cover here, or press return to skip"
  echo " >>> $URL"
  echo -n " >>> image url: "
  read COVER_URL
  if [ -z "$COVER_URL" ] ; then
    echo " >>> skipping."
  else
    COVER="${PREFIX} [CO].jpg"
    echo " >>> downloading image ..."
    $DOWNLOAD_CMD "$COVER" "$COVER_URL"

    echo " >>> adding new #COVER tag"
    mv "$TXT" "$TMP"
    echo "#COVER:${COVER}" | cat - "$TMP" > "$TXT"
    rm "$TMP"
  fi

fi

# BACKGROUND does not exist
if test -z "$BACKGROUND" || test ! -e "$BACKGROUND" ; then

  echo

  # remove BACKGROUND
  if [ -n "$BACKGROUND" ] ; then
    echo " >>> removing old #BACKGROUND tag"
    mv "$TXT" "$TMP"
    grep -v "#BACKGROUND:" "$TMP" > "$TXT"
    rm "$TMP"
  fi

  # download image
  ESCAPED=`escape "\"$ARTIST\" \"live performance\""`
  URL="http://images.google.com/images?q=$ESCAPED"
  echo " >>> Please look for an image of the band performing, or press return to skip"
  echo " >>> $URL"
  echo -n " >>> image url: "
  read BG_URL
  if [ -z "$BG_URL" ] ; then
    echo " >>> skipping."
  else
    BACKGROUND="${PREFIX} [BG].jpg"
    echo " >>> downloading image ..."
    $DOWNLOAD_CMD "$BACKGROUND" "$BG_URL"

    echo " >>> adding new #BACKGROUND tag"
    mv "$TXT" "$TMP"
    echo "#BACKGROUND:${BACKGROUND}" | cat - "$TMP" > "$TXT"
    rm "$TMP"
  fi

fi

# calculate first note offset
FIRST=`grep -E '[\\:F\\*] [0-9]+ [0-9]+ [0-9]+ ' "$TXT" | head -n 1 | cut -d " " -f 2`
SUBSTRACT=`echo "$FIRST / $BPM / 4 * 60000" | bc -l | cut -d . -f 1`

# all done, move files to destination
if [ "`cd .. ; pwd`" != "${USDB_DEST_DIR}" ] ; then
  DIR="${USDB_DEST_DIR}/$ARTIST - $TITLE"
  echo
  echo " >>> moving files to ..."
  echo " >>> $DIR/"
  mkdir "$DIR"
  mv "$TXT" "$MP3" "$DIR"
  [ -e "$VIDEO" ] && mv "$VIDEO" "$DIR"
  [ -e "$COVER" ] && mv "$COVER" "$DIR"
  [ -e "$BACKGROUND" ] && mv "$BACKGROUND" "$DIR"
else
  DIR="$WORK_DIR"
fi

echo
echo " >>> To get the timing right, you might want to do this:"
echo
echo "mplayer -ao pcm:file=/tmp/dump.wav -vo null \"$DIR/$MP3\" && ( gedit \"$DIR/$TXT\" & audacity /tmp/dump.wav ) && rm /tmp/dump.wav"
echo
if [ x$SUBSTRACT != x0 ] ; then
  echo " >>> Note that the first note does not start at zero."
  echo " >>> You have to substract ${SUBSTRACT}ms from the #GAP meassured with audacity."
fi

