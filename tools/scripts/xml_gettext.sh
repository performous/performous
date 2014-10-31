#!/bin/bash
#
# This script is intended to be executed from Poedit
# See instructions in lang/README.
#
# As long as the xml files are well-formed XML documents with
# entry blocks like the following one (from scheme.xml) this
# script should have no problems extracting the strings.
#		<entry ...>
#			<short>Karaoke mode</short>
#			<long>Hide pitch wave, notes and scoring.</long>
#		</entry>

# match <entry, ignoring allowed whitespace.
match_entry_start(){
	echo "$1" | grep -qE '^[[:space:]]*<[[:space:]]*entry.*>[[:space:]]*$'
	return $?
}

# match </entry>, ignoring allowed whitespace.
match_entry_end(){
	echo "$1" | grep -qE '^[[:space:]]*<[[:space:]]*/[[:space:]]*entry>[[:space:]]*$'
	return $?
}

# matches lines with <$1>...</$1>, ignoring allowed whitespace.
match_simple_tag(){
	echo "$2" | grep -qE "^[[:space:]]*<[[:space:]]*$1[[:space:]]*>.*</[[:space:]]*$1[[:space:]]*>[[:space:]]*$"
	return $?
}

# <abc>def</abc> -> _("def")
# Note: every " is automatically replaced by \". This is what you want, not
# invalid C code.
transform_simple_tag_to_keyworded_string(){
	echo "$1" | sed -re 's:\":\\\":g' -e 's:^[[:space:]]*<[[:space:]]*([a-z]+)[[:space:]]*>(.*)<[[:space:]]*/[[:space:]]*\1[[:space:]]*>[[:space:]]*$:\2:'
}

transform_and_add_string(){
	echo "#: $2:$3"
	echo "msgid \"$(transform_simple_tag_to_keyworded_string "$1")\""
	echo 'msgstr ""'
	echo
}

# transform <short/> and <long/> lines to _() lines
process_locale_block_line(){
	match_simple_tag "short" "$1" && transform_and_add_string "$1" "$2" "$3"
	match_simple_tag "long" "$1" && transform_and_add_string "$1" "$2" "$3"
}

process_xml(){
	IN_BLOCK=0
	line_no=0
	cat "$1" | while read line
	do
		line_no=$(($line_no + 1))

		# A simple 2-state automata, either we're in a <entry/>-block
		# or we're not. Limited detection and bail-out on malformed XML
		if [[ $IN_BLOCK -eq 1 ]] ; then
			match_entry_end "$line"
			if [[ $? -eq 0 ]] ; then
				IN_BLOCK=0
			else
				process_locale_block_line "$line" "$1" "$line_no"
			fi

			# <entry...> with out </entry> found:
			match_entry_start "$line" && (echo "Malformed XML $file:$line_no: Opening entry-tag found while already inside an entry block." >&2 ;exit 2)

		else
			# </entry> with out <entry> found:
			match_entry_end "$line" && (echo "Malformed XML $file:$line_no:: Closing entry-tag without prior opening tag." >&2 ;exit 2)

			match_entry_start "$line"
			if [[ $? -eq 0 ]] ; then
				IN_BLOCK=1
			fi
		fi
	done 
}


if [[ $# -lt 1 ]] ; then
	echo "USAGE: $0 <output file> [files...]"
	exit 1
fi

POEDIT_FILE="$1"
shift

# Start the dirty work
{
for file in $* ; do
	process_xml "$file"
done
} | msguniq -o "$POEDIT_FILE"


exit 0
