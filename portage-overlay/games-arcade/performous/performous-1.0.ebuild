# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /var/cvsroot/gentoo-x86/games-arcade/performous/performous-0.7.0-r1.ebuild,v 1.1 2013/01/24 17:04:10 hasufell Exp $

EAPI=5
[[ ${PV} = 9999 ]] && GIT="git-2"

CMAKE_REMOVE_MODULES="yes"
CMAKE_REMOVE_MODULES_LIST="FindALSA FindBoost FindGettext FindJpeg FindPng FindTiff FindZ"

inherit eutils base cmake-utils games ${GIT}

SONGS_PN=ultrastar-songs

DESCRIPTION="SingStar GPL clone"
HOMEPAGE="http://sourceforge.net/projects/performous/"
SRC_URI="songs? (
		mirror://sourceforge/performous/${SONGS_PN}-restricted-3.zip
		mirror://sourceforge/performous/${SONGS_PN}-jc-1.zip
		mirror://sourceforge/performous/${SONGS_PN}-libre-3.zip
		mirror://sourceforge/performous/${SONGS_PN}-shearer-1.zip
	)"

PATCHES=(
	"${FILESDIR}"/${PN}-20130811-gentoo.patch
	"${FILESDIR}"/${PN}-20140927-libav.patch
	"${FILESDIR}"/${PN}-20140927-linguas.patch
)

if [ "$PV" != "9999" ]; then
    SRC_URI="https://github.com/performous/performous/archive/${PV}.tar.gz -> ${P}.tar.gz $SRC_URI"
	PATCHES=(
		$PATCHES
		"${FILESDIR}"/${PN}-20140927-cmake.patch
	)
else
    EGIT_REPO_URI="git://github.com/performous/performous.git"
	PATCHES=(
		$PATCHES
		"${FILESDIR}"/${PN}-20141104-cmake.patch
	)
fi

LICENSE="GPL-2
	songs? (
		CCPL-Attribution-ShareAlike-NonCommercial-2.5
		CCPL-Attribution-NonCommercial-NoDerivs-2.5
	)"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE="midi songs tools webcam"

RDEPEND="dev-cpp/glibmm
	dev-cpp/libxmlpp
	media-libs/portaudio
	dev-libs/boost[threads(+)]
	dev-libs/glib
	dev-libs/libxml2
	gnome-base/librsvg
	tools? ( media-gfx/imagemagick )
	virtual/jpeg
	media-libs/libpng:0
	media-libs/libsdl2[joystick,video]
	virtual/ffmpeg
	virtual/opengl
	virtual/glu
	sys-libs/zlib
	virtual/libintl
	x11-libs/cairo
	x11-libs/gdk-pixbuf
	x11-libs/pango
	midi? ( media-libs/portmidi )
	webcam? ( media-libs/opencv )"
DEPEND="${RDEPEND}
	media-libs/glew
	sys-apps/help2man
	sys-devel/gettext"

src_prepare() {
	base_src_prepare
	sed -i \
		-e "s:@GENTOO_BINDIR@:${GAMES_BINDIR}:" \
		game/CMakeLists.txt \
		|| die

	strip-linguas -u lang
}

src_configure() {
	local mycmakeargs=(
		$(cmake-utils_use_enable tools TOOLS)
		$(usex midi "" "-DNO_PORTMIDI=ON")
		$(usex webcam "" "-DNO_WEBCAM=ON")
		-DCMAKE_VERBOSE_MAKEFILE=TRUE
		-DSHARE_INSTALL="${GAMES_DATADIR}"/${PN}
	)
	cmake-utils_src_configure
}

src_compile() {
	cmake-utils_src_compile
}

src_install() {
	cmake-utils_src_install
	if use songs ; then
		insinto "${GAMES_DATADIR}"/${PN}
		doins -r "${WORKDIR}/songs"
	fi
	dodoc docs/{Authors,instruments}.txt
	prepgamesdirs
}
