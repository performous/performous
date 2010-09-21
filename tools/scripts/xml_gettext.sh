#!/bin/bash
#
# This script is intended to be executed from Poedit
# See instructions in lang/README.
#
# As long as the xml files are well-formed XML documents with
# locale blocks like the following one (from scheme.xml) this
# script should have no problems extracting the strings.
#		<locale name="C">
#			<short>Karaoke mode</short>
#			<long>Hide pitch wave, notes and scoring.</long>
#		</locale>

# adds a line to the temporary source file
append_temp_src(){
	echo "$*" >> "$TEMP_SRC"
}

# match <locale name="C">, ignoring allowed whitespace.
match_locale_start(){
	echo "$1" | grep -qE '^[[:space:]]*<[[:space:]]*locale[[:space:]]+name="C"[[:space:]]*>[[:space:]]*$'
	return $?
}

# match </locale>, ignoring allowed whitespace.
match_locale_end(){
	echo "$1" | grep -qE '^[[:space:]]*<[[:space:]]*/[[:space:]]*locale>[[:space:]]*$'
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
	echo "$1" | sed -re 's:\":\\\":g' -e 's:^[[:space:]]*<[[:space:]]*([a-z]+)[[:space:]]*>(.*)<[[:space:]]*/[[:space:]]*\1[[:space:]]*>[[:space:]]*$:_("\2"):'
}

transform_and_add_string(){
	append_temp_src "$(transform_simple_tag_to_keyworded_string "$1")"
}

# transform <short/> and <long/> lines to _() lines
process_locale_block_line(){
	match_simple_tag "short" "$1" && transform_and_add_string "$1"
	match_simple_tag "long" "$1" && transform_and_add_string "$1"
}

process_xml(){
	IN_BLOCK=0
	line_no=0
	cat "$1" | while read line
	do
		line_no=$(($line_no + 1))

		# A simple 2-state automata, either we're in a <locale/>-block
		# or we're not. Limited detection and bail-out on malformed XML
		if [[ $IN_BLOCK -eq 1 ]] ; then
			match_locale_end "$line"
			if [[ $? -eq 0 ]] ; then
				IN_BLOCK=0
			else
				process_locale_block_line "$line"
			fi

			# <locale...> with out </locale> found:
			match_locale_start "$line" && (echo "Malformed XML $file:$line_no: Opening locale-tag found while already inside a locale block." >&2 ;exit -1)

		else
			# </locale> with out <locale> found:
			match_locale_end "$line" && (echo "Malformed XML $file:$line_no:: Closing locale-tag without prior opening tag." >&2 ;exit -1)

			match_locale_start "$line"
			if [[ $? -eq 0 ]] ; then
				IN_BLOCK=1
			fi
		fi
	done 
}


if [[ $# -lt 2 ]] ; then
	echo "USAGE: $0 <output file> <input encoding> [files...]"
	exit -1 
elif [[ $# -eq 2 ]] ; then
	# no input files nothing to do
	exit 0
fi

POEDIT_FILE="$1"
ENC="$2"
shift
shift

# create the temporary file securely.
TEMP_SRC=$(mktemp --tmpdir xml2gettext.XXXXXXXXXX).c
append_temp_src "/* This is a automatically generated temp file, it's safe to remove*/"

# Start the dirty work
for file in $* ; do
	process_xml "$file"
done

# Invoke xgettext, poedit will merge this with the rest of the strings
xgettext --force-po -L C -o "$POEDIT_FILE" --from-code="$ENC" -k_ "$TEMP_SRC"
RV=$?

# clean up
rm "$TEMP_SRC"

exit $RV
