#!/bin/bash
#
# TODO: error handling, security, the usual suspects.

# Keep these in the global scope
TOT=
UNT=
FUZ=
TPC=
ISO_639_XML="/usr/share/xml/iso-codes/iso_639.xml"

die(){
    echo "$*" >&2
    exit -1
}

show_usage(){
    echo "USAGE: $0 <--overview|--infoboxes [lang]> [--iso-639 file]"
    echo
    echo " --overview           Generates the language overview table."
    echo " --infoboxes [lang]   The two letter ISO-369-1 langauge code used in the PO"
    echo "                       filename, or the 3-letter terminology code (ISO-639-2T),"
    echo "                       if no two letter code exits. If none is given infoboxes"
    echo "                       for all languages found is generated."
    echo " [--iso-639 file]   Override the location of the ISO-639-1 XML database."
    echo "                       This defaults to: /usr/share/xml/iso-codes/iso_639.xml"
}

if [[ "$(basename $PWD)" != "performous" ]] || [[ ! -d ".git" ]] ; then
	echo "This script must be run from the perfourmous source folder"
	die "Error: Not in the performous root dir!"
fi

# Colorize
#   100 => green background + bold face
#  >=95 => green background
#  >=75 => orangeish background
#  >=50 => orageish background
#  >=25 => red background
#   <25 => red background
colorize(){
	echo -n 'style="background-color: '
	if [[ $1 -eq 100 ]] ; then
		echo -n "#00FF00; font-weight: bold"
	elif [[ $1 -ge 95 ]] ; then
		echo -n "#00FF00"
	elif [[ $1 -ge 75 ]] ; then
		echo -n "#CCEE00"
	elif [[ $1 -ge 50 ]] ; then
		echo -n "#CC9900"
	elif [[ $1 -ge 25 ]] ; then
		echo -n "#CC0000"
	else
		echo -n "#990000"
	fi
	echo -n ';" |'
}

# This assumes the file /usr/share/xml/iso-codes/iso_639.xml exists
# and has a very strict format (not only valid XML!)
get_full_language_name(){
	# We have no good solution for pt_BR yet
	L_ONLY="$(echo "$1" | sed 's/_[A-Za-z]*//')"

	if [[ ! -f "$ISO_639_XML" ]] ; then
		echo "<!> Missing file \"$(basename "$ISO_639_XML")\""
		die "FATAL ERROR: Missing ISO-639 mappings."
	fi

	# Note assumptions about the XML file formatting:
	# (lines marked with * are the important ones, -/... is ignored data)
	# - <iso_639_entry                      - <iso_639_entry
	# - ...                                 - ...
	# * iso_639_1_code="xx"     -- XOR --   * iso_639_2T_code="xxx"
	# * name="Xyzuvw; ..." />               * name="Xyzuvw; ..." />

	# only pick the first name if many (see XML for examples on eg "nl")
	NAME=$(grep -A1 "iso_639_1_code=\"$L_ONLY\"" "$ISO_639_XML" | grep name | sed -re 's:[ \t]*name="([^;\"]+).*:\1:')
	if [[ -z "$NAME" ]] ; then
	    # Try the 3-letter terminology form before giving up
	    # <http://www.opentag.com/xfaq_lang.htm>
	    NAME=$(grep -A1 "iso_639_2T_code=\"$L_ONLY\"" "$ISO_639_XML" | grep name= | sed -re 's:[ \t]*name="([^;\"]+).*:\1:') 

	    if [[ -z "$NAME" ]] ; then
			# causes: no 2 or 3 letter code exists, or both exists and the 3 letter
			# version was used (causing -A1 to give the iso_639_1_code line, grepping
			# for name= will fail => NAME="")

			# Fall back to something other than an empty string
			NAME="Unknown"
	    fi
	fi
	echo "$NAME"
}

# get_translator_field: ISO-369-1 Lang × STRING -> STRING
get_translator_fields(){
	DB="lang/TRANSLATORS"
	if [[ -f "$DB" ]] ; then
		grep "$1:$2 " "$DB" | sed "s%$1:$2 %%"
		return $?
	fi
	echo ""
	return -1
}

get_translator_comment(){
	get_translator_fields "$1" "COMMENT"
	return $?
}

last_commit_date(){
	# ci vs ai? author vs commit date? I'm guessing ci is what we want,
	# when the commit was made, not when the autor made the edit.
	# (thus the assumption is: %ci more recent or exactly equal to %ai)
	git log --date-order -n1 --pretty=format:%ci --no-color --date=iso
}
last_commit_id(){
	# %H = full commit hash
	# %h = short commit hash
	git log --date-order -n1 --pretty=format:%h --no-color --date=iso
}
last_commit_gitweb_uri(){
	local GIT_WEB_URI_BASE="https://github.com/performous/performous"
	local GIT_WEB_URI="${GIT_WEB_URI_BASE}/commit/$(git log --date-order -n1 --pretty=format:%H --no-color --date=iso)"
	echo "$GIT_WEB_URI"
}

current_gitweb_uri(){
	local GIT_WEB_URI_BASE="https://github.com/performous/performous/tree/master"
	local GIT_WEB_URI="${GIT_WEB_URI_BASE}/$1"
	echo "$GIT_WEB_URI"
}

# update_vars : ISO-369-1 LANG -> Ø
update_vars(){
	OUT="$( LC_ALL=C msgfmt --statistics "lang/$1.po" 2>&1 )"

	# Extract translated, fuzzy and untranslated counts
	# Note: msgfmt has bad habits:
	#  * stats are apparently errors, as they're sent to stderr
	#  * it says "1 fuzzy translation" and "2 fuzzy translations", thus
	#    the "...s?..." to catch that case, I'm assuming this holds for
	#    all three components.
	#sed -re "s%^([0-9]+) translated messages?(, ([0-9]+) fuzzy translations?)?(, ([0-9]+) untranslated messages?)?\.%LANG=$1;T=\1;F=\3;U=\5%"
	TOT=$(echo "$OUT" | sed -re "s%^([0-9]+) translated messages?(, ([0-9]+) fuzzy translations?)?(, ([0-9]+) untranslated messages?)?\.%\1%")
	FUZ=$(echo "$OUT" | sed -re "s%^([0-9]+) translated messages?(, ([0-9]+) fuzzy translations?)?(, ([0-9]+) untranslated messages?)?\.%\3%")
	UNT=$(echo "$OUT" | sed -re "s%^([0-9]+) translated messages?(, ([0-9]+) fuzzy translations?)?(, ([0-9]+) untranslated messages?)?\.%\5%")

	# compute the % of translated strings
	TPC="N/A"
	if 	[[ -z "$TOT" ]] || [[ "$TOT" == "0" ]] &&
		[[ -z "$FUZ" ]] || [[ "$FUZ" == "0" ]] &&
		[[ -z "$UNT" ]] || [[ "$UNT" == "0" ]] ; then
		TPC="0"
	else
		[[ -z "$TOT" ]] && TOT=0
		[[ -z "$FUZ" ]] && FUZ=0
		[[ -z "$UNT" ]] && UNT=0
		TPC="$(echo "100 * $TOT / (( $TOT + $UNT + $FUZ ))" | bc)"
	fi
}

# internal use by generate_overview
overview_wikify(){
	I_LANG="$(echo "$1" | sed -re 's:lang/([a-zA-Z_]+)\.po:\1:')"
	YYYY_MM_DD="$(LC_ALL=C git log -1 --format="%ar" lang/$I_LANG.po)"
	LONG_LANG="$(get_full_language_name "$I_LANG")"
	[[ $? -eq 0 ]] || die "$LONG_LANG"
	update_vars "$I_LANG"

	# find overview translation comments (if any)
	COMMENT="$(get_translator_comment $I_LANG)"
	PC_COLOR=$(colorize $TPC)

	echo "|-"
	echo "| [[Translations ($LONG_LANG)|$LONG_LANG ($I_LANG)]] || $YYYY_MM_DD || ${PC_COLOR} ${TPC}&nbsp;% || $TOT || $FUZ || ${UNT} || '''$(( $TOT + $UNT + $FUZ ))''' || [$(current_gitweb_uri "$1") $I_LANG.po] || ${COMMENT}"
}

generate_overview(){
	cat <<EOH
{| border="1" cellspacing="0" cellpadding="5"
| rowspan="2" | '''Language'''                || rowspan="2" | '''Last update''' || colspan="5" align="center" | '''Progress''' || rowspan="2" | '''Current version''' || rowspan="2" | '''Comment'''
|-
|                                                                       '''%''' || '''Translated''' || '''Fuzzy''' || '''Untranslated''' || '''Total'''
EOH
	for a in $(ls -1 lang/*po) ; do
		overview_wikify "$a"
	done
	echo "|}"

	echo "Table last updated: $(date --rfc-3339=seconds) (based on git commit <tt>[$(last_commit_gitweb_uri) $(last_commit_id)]</tt> @ $(last_commit_date))"
}

generate_infoboxes(){
	I_LANG="$(echo "$1" | sed -re 's:lang/([a-z]+)\.po:\1:')"
	YYYY_MM_DD="$(LC_ALL=C git log -1 --format="%ci" lang/$I_LANG.po)"

	OUT="$( LC_ALL=C msgfmt --statistics "$1" 2>&1 )"
	LONG_LANG="$(get_full_language_name "$I_LANG")"
	[[ $? -eq 0 ]] || die "$LONG_LANG"
	update_vars "$I_LANG"

	STATUS="$(get_translator_fields "$I_LANG" "STATUS")"
	CONTACT="$(get_translator_fields "$I_LANG" "CONTACT")"

	echo "{{Translation:TranslationInfo|Status=$STATUS|LANG=$I_LANG|PERCENT=$TPC|STRINGS=$TOT / $(( $TOT + $FUZ + $UNT ))|FUZZY=$FUZ|NOTRANS=$UNT|Updated=$YYYY_MM_DD|Contact=$CONTACT}}"
}

if [[ "$1" == "--overview" ]] ; then
    if [[ "$2" == "--iso-639" ]] && [[ -n "$3" ]] ; then
	    [[ -f "$3" ]] || die "The <$3> doesn't exist!"
	    ISO_639_XML="$3"
	elif [[ $# -ne 1 ]] ; then
	    show_usage
	    die "Error: Invalid commandline."
	fi
	generate_overview
elif [[ "$1" == "--infoboxes" ]] ; then
    L="$2"

	shift # of the --infoboxes arg
	if [[ "$L" == "--iso-639" ]] ; then
	    # lang to --infoboxes was omitted, clear it.
	    L=""
	else
	    shift # of the lang argument
	fi

	if [[ $# -eq 2 ]] && [[ "$1" == "--iso-639" ]] && [[ -n "$1" ]] ; then
	    [[ -f "$2" ]] || die "The <$2> doesn't exist!"
	    ISO_639_XML="$2"
	elif [[ $# -ne 0 ]] ; then
	    show_usage
	    die "Error: Invalid commandline."
	fi

	if [[ -z "$L" ]] ; then
		for a in $(ls -1 lang/*po) ; do
			generate_infoboxes "$a"
		done
	else
	    if [[ -d lang ]] && [[ ! -f "lang/${L}.po" ]] ; then
			die "There is no translation (.po) file for <$L>!"
		fi
		generate_infoboxes "$L"
	fi
else
    show_usage
fi
