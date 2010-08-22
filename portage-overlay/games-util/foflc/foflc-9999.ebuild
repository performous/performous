# Copyright 1999-2010 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=3

inherit cmake-utils games subversion

DESCRIPTION="Frets On Fire Lyrics Converter"
HOMEPAGE="http://www.fretsonfire.net/forums/viewtopic.php?t=31765&f=11"
ESVN_REPO_URI="http://editor-on-fire.googlecode.com/svn/trunk/src/foflc"

LICENSE="ASIS"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE=""

DEPEND=""
RDEPEND="${DEPEND}"

src_configure() {
	mycmakeargs="-DCMAKE_INSTALL_PREFIX=${GAMES_PREFIX}"
	cmake-utils_src_configure
}

src_compile() {
	cmake-utils_src_compile
}

src_install() {
	DOCS="*.txt" cmake-utils_src_install
	prepgamesdirs
}
