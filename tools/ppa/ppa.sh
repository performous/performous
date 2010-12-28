#!/bin/bash
# This script fetches fresh git version, applies
# the changes on top of the current source package
# from the repositories, builds and signs the new
# package and uploads it to Launchpad PPA.

#TODO: better changelog (from git?)


set -e   # Exit on fail

# Assume default GPG key setup and use it for name & email.
# Export the vars because the tools use these magic variables also.
export DEBFULLNAME="`gpg --list-keys | grep uid | sed 's/ <.*//; s/.*uid *//'`"
export DEBEMAIL="`gpg --list-keys | grep uid | sed 's/ *(.*)//; s/>.*//; s/.*[:<] *//'`"

# Config
PKG="performous"
VERSIONCOMMON="0.6.1-99+git"`date '+%Y%m%d'`"~ppa1"
BASEPKGVERSION="0.6.1" # base version of the base package
BASEPKGADD="-1" # additional version suffix of the base package
BASEURL="http://archive.ubuntu.com/ubuntu/pool/universe/p/performous"
SUITES="lucid maverick natty"
GITURL="git://git.performous.org/gitroot/performous/performous"
DESTINATIONPPA="ppa:performous-team/ppa"

TEMPDIR=`mktemp -dt $PKG-ppa.XXXXXXXXXX`
SOURCEDIR="$TEMPDIR/git"
PPAPATCHDIR="`pwd`"

	# Print a status message
	status()
	{
		echo -e "\e[0;31m"$@"\e[0m"
	}

	# Copy the new files for source package
	CopyNewFiles()
	{
		COPYCMD="cp -r"
		$COPYCMD "$1/CMakeLists.txt" "$2"
		$COPYCMD "$1/README.txt" "$2"
		$COPYCMD "$1/cmake" "$2"
		$COPYCMD "$1/data" "$2"
		$COPYCMD "$1/docs" "$2"
		$COPYCMD "$1/game" "$2"
		$COPYCMD "$1/lang" "$2"
		$COPYCMD "$1/themes" "$2"
		$COPYCMD "$1/tools" "$2"
	}

cd "$TEMPDIR"
status "Tempdir: `pwd`"

# Figure out the version of the "old" official package
status "Working on $PKG ${BASEPKGVERSION}${BASEPKGADD}"

mkdir -p $PKG-$BASEPKGVERSION
cd $PKG-$BASEPKGVERSION

# Download the "old" source package we use as a base
wget $BASEURL/${PKG}_${BASEPKGVERSION}${BASEPKGADD}.dsc
wget $BASEURL/${PKG}_${BASEPKGVERSION}.orig.tar.bz2
wget $BASEURL/${PKG}_${BASEPKGVERSION}${BASEPKGADD}.debian.tar.bz2

# Download fresh version from git
status "Fetch from git..."
git clone "$GITURL" "$SOURCEDIR"

# Get some info from git for changelog
(
	cd "$SOURCEDIR"
	# 10 chars from the HEAD commit hash
	headcommit=`git log | head -n 1 | cut --delimiter=" " -f 2 | cut -c 1-10`
)

# Loop suites
status "Do each suite..."
for suite in $SUITES ; do
	newversion="${VERSIONCOMMON}~${suite}"
	rm -rf $suite; mkdir $suite
	(
		cd $suite
		status "Extracting source for $suite..."
		ln ../${PKG}_* .
		dpkg-source -x ${PKG}_${BASEPKGVERSION}${BASEPKGADD}.dsc extracted
		(
			cd extracted

			# Copy new files
			status "Copy new files..."
			CopyNewFiles "$SOURCEDIR" .

			# Apply patches
			status "Apply some patches..."
			cp "$PPAPATCHDIR/"*.patch .
			for p in *.patch; do
				patch -p0 < "$p"
			done
			rm *.patch

			# Hack hack
			#TODO: Get rid of these
			status "Apply some hacks..."
			rm -rf debian/patches # Delete troublesome patch

			# Do changelog
			# Dch complains about unknown suites
			yes '' | dch -b -v $newversion -D $suite "Upload development version from Git $headcommit to Ubuntu PPA for $suite."

			# Build package
			#dpkg-buildpackage -sa -S    # Full .orig.gz
			dpkg-buildpackage -sd -S    # Only .diff.gz
		)
		# Upload to PPA
		dput $DESTINATIONPPA ${PKG}_${newversion}_source.changes
	)
done

status "Files were kept in $TEMPDIR"

