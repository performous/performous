#!/bin/sh

. $GITDIR/hooks/functions.bash

process()
{
	file="$1"

	mv "$file" "$file.orig"
	cat "$file.orig" | sed 's/[ \t]\+$//g' > "$file"
	rm "$file.orig"
	git add "$file"
}

changed_files=`getChangedFiles`

if [ -n "$changed_files" ]
then
	echo "$changed_files" | while read file
	do
		echo "process \"$file\""
		process "$file"
	done
fi

exit 0
