#!/bin/bash
#
# TODO: error handling, security, the usual suspects.

# Keep these in the global scope
TOT=
UNT=
FUZ=
TPC=

if [[ "$(basename $PWD)" != "performous" ]] || [[ ! -d ".git" ]] ; then
	echo "Error: Not in the performous root dir!"
	echo "This script must be run from the perfourmous source folder"
	exit
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
	if [[ ! -f /usr/share/xml/iso-codes/iso_639.xml ]] ; then
		echo "<!> Missing /usr/share/xml/iso-codes/iso_639.xml"
		echo "FATAL ERROR: Missing ISO-639-1 mappings."
		exit -1
	fi

	# only pick the first name if many (see XML for examples on eg "nl")
	grep -A1 "iso_639_1_code=\"$1\"" /usr/share/xml/iso-codes/iso_639.xml | grep name | sed -re 's:[ \t]*name="([^;\"]+).*:\1:'
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
	# SF_GIT_WEB_URI
	SF_GIT_WEB_URI_BASE="http://performous.git.sourceforge.net/git/gitweb.cgi?p=performous/performous;a=commit"
	SF_GIT_WEB_URI="${SF_GIT_WEB_URI_BASE};h=$(git log --date-order -n1 --pretty=format:%H --no-color --date=iso)"
	echo "$SF_GIT_WEB_URI"
}

current_gitweb_uri(){
	SF_GIT_WEB_URI_BASE="http://performous.git.sourceforge.net/git/gitweb.cgi?p=performous/performous;a=blob;hb=HEAD"
	SF_GIT_WEB_URI="${SF_GIT_WEB_URI_BASE};f=$1"
	echo "$SF_GIT_WEB_URI"
}

# update_vars : ISO-369-1 LANG -> Ø
update_vars(){
	OUT="$( LC_ALL=C msgfmt --statistics "lang/$1.po" 2>&1 )"

	# Extract translated, fuzzy and untranslated counts
	# Note: msgfmt has bad habits:
	#  * stats are apparently errors, as they're sent to stderr
	#  * it says "1 fuzzy translation" andt "2 fuzzy translations", thus
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
	YYYY_MM_DD="$(LC_ALL=C date +%F)"
	I_LANG="$(echo "$1" | sed -re 's:lang/([a-z]+)\.po:\1:')"
	LONG_LANG="$(get_full_language_name "$I_LANG")"
	update_vars "$I_LANG"

	# find overview translation comments (if any)
	COMMENT="$(get_translator_comment $I_LANG)"
	PC_COLOR=$(colorize $TPC)

	echo "|-"
	echo "|   [[Translations ($LONG_LANG)|$I_LANG]]  || $YYYY_MM_DD  || ${PC_COLOR} ${TPC} % || $TOT || $FUZ || ${UNT} || '''$(( $TOT + $UNT + $FUZ ))''' || [$(current_gitweb_uri "$1") $1] || ${COMMENT}"
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
	YYYY_MM_DD="$(LC_ALL=C date +%F)"

	OUT="$( LC_ALL=C msgfmt --statistics "$1" 2>&1 )"
	I_LANG="$(echo "$1" | sed -re 's:lang/([a-z]+)\.po:\1:')"
	LONG_LANG="$(get_full_language_name "$I_LANG")"
	update_vars "$I_LANG"

	STATUS="$(get_translator_fields "$I_LANG" "STATUS")"
	CONTACT="$(get_translator_fields "$I_LANG" "CONTACT")"

	echo "{{Translation:TranslationInfo|Status=$STATUS|LANG=$I_LANG|PERCENT=$TPC|STRINGS=$TOT / $(( $TOT + $FUZ + $UNT ))|FUZZY=$FUZ|NOTRANS=$UNT|Updated=$YYYY_MM_DD|Contact=$CONTACT}}"
}

if [[ "$1" == "--overview" ]] ; then
	generate_overview
elif [[ "$1" == "--infoboxes" ]] ; then
	if [[ -z "$2" ]] ; then
		for a in $(ls -1 lang/*po) ; do
			generate_infoboxes "$a"
		done
	else
	    	if [[ -d lang ]] && [[ ! -f "lang/${2}.po" ]] ; then
		    echo "There is no translation (.po) file for <$2>!"
		    exit -1
		fi
		generate_infoboxes "$2"
	fi
else
	echo "USAGE: $0 <--overview|--infoboxes [lang]>"
	echo
	echo " --overview           Generates the language overview table."
	echo " --inforboxes [lang]  The two letter ISO-369-1 langauge code used in the PO"
	echo "                       filename. If none is given infoboxes for all languages"
	echo "                       found is generated."
fi
