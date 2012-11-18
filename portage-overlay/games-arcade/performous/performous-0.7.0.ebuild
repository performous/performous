# Copyright 1999-2012 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=4

[[ ${PV} = 9999 ]] && GIT="git-2"

inherit cmake-utils ${GIT} games

SONGS_PN=ultrastar-songs

DESCRIPTION="Party game similar to Singstar, RockBand, Guitar Hero and Stepmania"
HOMEPAGE="http://performous.org"
SRC_URI="songs? ( 
	mirror://sourceforge/${PN}/${SONGS_PN}-jc-1.zip
	mirror://sourceforge/${PN}/${SONGS_PN}-libre-3.zip
	mirror://sourceforge/${PN}/${SONGS_PN}-restricted-3.zip
	mirror://sourceforge/${PN}/${SONGS_PN}-shearer-1.zip
	)"

if [ "$PV" != "9999" ]; then
	SRC_URI=" mirror://sourceforge/${PN}/${P}.tar.bz2
		$SRC_URI"
else
	EGIT_REPO_URI="git://performous.git.sourceforge.net/gitroot/performous/performous"
	# git-2 default branch is master
	#EGIT_BRANCH="master"
	# use performous_LIVE_BRANCH env var to install another branch (for example
	# legacy or torrent)
fi

LICENSE="GPL-2
	songs? (
		CCPL-Attribution-ShareAlike-NonCommercial-2.5
		CCPL-Attribution-NonCommercial-NoDerivs-2.5
	)"
SLOT="0"
KEYWORDS="~amd64 ~x86"

IUSE="debug midi songs tools webcam"

RDEPEND="gnome-base/librsvg
	>=dev-libs/boost-1.39.0
	x11-libs/pango
	dev-cpp/libxmlpp
	media-libs/glew
	media-libs/libsdl[joystick,opengl]
	media-libs/libpng
	virtual/jpeg
	tools? ( media-gfx/imagemagick[png] )
	midi? ( media-libs/portmidi )
	webcam? ( media-libs/opencv[v4l] )
	>=media-video/ffmpeg-0.4.9_p20070616-r20
	media-libs/portaudio
	sys-apps/help2man
	!games-arcade/ultrastar-ng"
DEPEND="${RDEPEND}
    >=dev-util/cmake-2.6.0"

src_unpack() {
	if [ "${PV}" != "9999" ]; then
		unpack "${P}.tar.bz2"
	else
		git-2_src_unpack
	fi
	cd "${S}"
	if use songs; then
		unpack "${SONGS_PN}-jc-1.zip" "${SONGS_PN}-libre-3.zip" "${SONGS_PN}-restricted-3.zip" "${SONGS_PN}-shearer-1.zip"
	fi
}

src_prepare() {
	epatch "${FILESDIR}"/${PN}-gentoopaths.patch
}

src_configure() {
	local mycmakeargs="
		$(cmake-utils_use_enable tools TOOLS)
		$(cmake-utils_use_no webcam WEBCAM)
		$(cmake-utils_use_no midi PORTMIDI)
		-DCMAKE_INSTALL_PREFIX=${GAMES_PREFIX}
		-DGENTOO_DATA_DIR=${GAMES_DATADIR}/${PN}
		-DLOCALE_DIR=/usr/share
		-DCMAKE_BUILD_TYPE=Release"

	cmake-utils_src_configure
}

src_compile() {
	cmake-utils_src_compile
}

src_install() {
	DOCS="docs/*.txt" cmake-utils_src_install
	mv -f "${D}/${GAMES_PREFIX}/share/man" "${D}/usr/share/"
	mkdir -p "${D}/${GAMES_DATADIR}/${PN}"
	mv -f "${D}/${GAMES_PREFIX}/share/games/performous" "${D}/${GAMES_DATADIR}/"

	if use songs; then
		insinto "${GAMES_DATADIR}/${PN}"
		doins -r "${S}/songs" || die "doins songs failed"
	fi
	doicon "${S}/data/${PN}.xpm"
	domenu "${S}/data/${PN}.desktop"
	prepgamesdirs
}
