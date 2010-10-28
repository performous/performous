# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /cvsroot/ultrastar-ng/UltraStar-ng/portage-overlay/games-arcade/performous/performous-9999.ebuild,v 1.10 2007/09/29 13:04:19 yoda-jm Exp $

[[ ${PV} = 9999 ]] && GIT="git"
EAPI=2

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
	MY_PN=Performous
	MY_P=${MY_PN}-${PV}-Source
	SRC_URI=" mirror://sourceforge/${PN}/${MY_P}.tar.bz2
		$SRC_URI"
else
	EGIT_REPO_URI="git://performous.git.sourceforge.net/gitroot/performous/performous"
	EGIT_BRANCH="master"
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
	media-libs/jpeg
	tools? ( media-gfx/imagemagick[png] )
	webcam? ( media-libs/opencv[v4l] )
	>=media-video/ffmpeg-0.4.9_p20070616-r20
	media-libs/portaudio
	sys-apps/help2man
	!games-arcade/ultrastar-ng"
# Waiting for portmidi to enter portage (#90614)
#RDEPEND="${RDEPEND}
#	midi? ( media-libs/portmidi )"
DEPEND="${RDEPEND}
    >=dev-util/cmake-2.6.0"

S=${WORKDIR}/${MY_P}

src_unpack() {
	if [ "${PV}" != "9999" ]; then
		unpack "${MY_P}.tar.bz2"
	else
		git_src_unpack
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
		-DCMAKE_INSTALL_PREFIX=${GAMES_PREFIX}
		-DGENTOO_DATA_DIR=${GAMES_DATADIR}/${PN}
		-DLOCALE_DIR=/usr/share
		-DCMAKE_BUILD_TYPE=Release"
#	local mycmakeargs="
#		$(cmake-utils_use_no midi MIDI)
#		${mycmakeargs}"

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
