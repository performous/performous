#/bin/sh
# this script execute "./setvocals_one_song.sh" recursively
# usage ./setvocals_recursively.sh [parentdir]
# If you run it with no arguments, it will use the current dir as parent dir


[ $# -gt 0 ] || set -- .;
find "$@" -type d -exec sh ./setvocals.sh {} \;
