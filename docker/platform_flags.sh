#!/bin/bash

if [[ ! -f /root/performous/platform_flags.sh ]]; then
	mkdir -p /root/performous
	cp -v ./platform_flags.sh /root/performous/
fi

function export_platform_flags () {
	if [[ $# > 1 ]]; then
		echo "WARNING: export_platform_flags takes only 1 argument, the rest are ignored." 
	elif [[ $# < 1 ]]; then
		echo "ERROR: export_platform_flags needs an argument."
		exit 1
	fi
		echo "export PLATFORM_CMAKE_FLAGS='"${1}"'" | tee -a /root/performous/PLATFORM_CMAKE_FLAGS
}

function import_platform_flags () {
	if [ $# -gt 0 ]; then
		echo "WARNING: import_platform_flags does not take arguments, we'll just ignore them." 
	elif [ ! -d /root/performous ]; then
		echo "ERROR: /root/performous doesn't exist, so there can't be any flags to import. Use `export_platform_flags` first."
		exit 1
	fi
	test -f /root/performous/PLATFORM_CMAKE_FLAGS && . /root/performous/PLATFORM_CMAKE_FLAGS
}