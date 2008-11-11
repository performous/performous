#/bin/sh
# this script converts cp1252(windows) filenames+texts to utf8 and fixes wrong .mp3 filenames
# usage ./fix-files.sh [songdir]

cd "$1"

function fenc {
 convmv -f cp1252 -t utf8 --notest ./*
 echo '- converted filenames to utf8'
}

function textenc {
 find ./ | grep ".txt" | grep -v .cp1252 | while read txts; do
  if [ -f "$txts.cp1252" ];
  then
   echo "- '$txts.cp1252' exists. skipping.."
  else
   mv "$txts" "$txts.cp1252"
   iconv --from-code=ISO-8859-1 --to-code=UTF-8 "$txts.cp1252" > "$txts"
   dos2unix "$txts"
   echo "- converted $txts to utf8 and created backup $txts.cp1252"
 fi
 done
}

function mp3name {
 txt=`find ./ | grep - | grep .txt | grep -v .txt.`
 mp3name=`cat "$txt" | grep MP3 | sed -e s/#MP3:// -e s/"\r"//g`
 oldfile=`find ./ | grep - | grep -i .mp3`
  if [ -f "$mp3name" ];
  then
   echo "- '$mp3name already exists'. check usng log for errors if you still can't select it"
  else
   mv "$oldfile" "$mp3name"
   echo "- moved '$oldfile' to '$mp3name'"
  fi
}

fenc
textenc
mp3name
