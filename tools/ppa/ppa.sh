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
VERSIONCOMMON="0.5.1-9+git"`date '+%Y%m%d'`"~ppa1"
SUITES="karmic lucid maverick"
GITURL="git://git.performous.org/gitroot/performous/performous"
DESTINATIONPPA="ppa:performous-team/ppa"

TEMPDIR=`mktemp -dt $PKG-ppa.XXXXXXXXXX`
SOURCEDIR="$TEMPDIR/git"
PPAPATCHDIR="`pwd`"

	# Copy the new files for source package
	CopyNewFiles()
	{
		COPYCMD="cp -r"
		$COPYCMD "$1/CMakeLists.txt" "$2"
		$COPYCMD "$1/cmake" "$2"
		$COPYCMD "$1/data" "$2"
		$COPYCMD "$1/docs" "$2"
		$COPYCMD "$1/editor" "$2"
		$COPYCMD "$1/game" "$2"
		$COPYCMD "$1/lang" "$2"
		$COPYCMD "$1/themes" "$2"
		$COPYCMD "$1/tools" "$2"
		rm -rf "$2"/libs   # Old libs dir not used anymore
	}

cd "$TEMPDIR"

# Figure out the version of the "old" official package
version=`apt-cache showsrc $PKG | sed -n 's/^Version: \(.*\)/\1/p' | head -n 1`
echo "Working on $PKG $version"

mkdir -p $PKG-$version
cd $PKG-$version

# Download the "old" source package we use as a base
apt-get --download-only source $PKG

# Download fresh version from git
echo "Fetch from git..."
git clone "$GITURL" "$SOURCEDIR"

# Get some info from git for changelog
pushd .
cd "$SOURCEDIR"
# 10 chars from the HEAD commit hash
headcommit=`git log | head -n 1 | cut --delimiter=" " -f 2 | cut -c 1-10`
popd

# Loop suites
for suite in $SUITES ; do
	newversion="${VERSIONCOMMON}~${suite}"
	rm -rf $suite; mkdir $suite
	cd $suite
	ln ../${PKG}_* .
	dpkg-source -x ${PKG}_${version}.dsc extracted
	cd extracted
		# Copy new files
		echo "Copy new files..."
		CopyNewFiles "$SOURCEDIR" .
		# Apply patches
		echo "Apply some patches..."
		cp "$PPAPATCHDIR/"*.patch .
		patch -p0 < *.patch
		rm *.patch

		# Do changelog
		# Dch complains about unknown suites
		yes '' | dch -b -v $newversion -D $suite "Upload development version from Git $headcommit to Ubuntu PPA for $suite."
		if [ -f debian/source/format -a ! -f debian/patches/series ] ; then
			rm debian/source/format
		fi
		# Build package
		#dpkg-buildpackage -sa -S    # Full .orig.gz
		dpkg-buildpackage -sd -S    # Only .diff.gz
	cd ..
	# Upload to PPA
	dput $DESTINATIONPPA ${PKG}_${newversion}_source.changes
	cd ..
done
cd ..

echo "Files were kept in $TEMPDIR"

