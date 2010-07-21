# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /cvsroot/ultrastar-ng/UltraStar-ng/portage-overlay/games-arcade/performous/performous-9999.ebuild,v 1.10 2007/09/29 13:04:19 yoda-jm Exp $

EAPI=2

inherit games cmake-utils
[ "$PV" == "9999" ] && inherit git

RESTRICT="nostrip"

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
KEYWORDS="~x86 ~amd64 ~ppc ~ppc64"

IUSE="debug alsa portaudio pulseaudio jack songs gstreamer tools editor midi webcam"

RDEPEND="gnome-base/librsvg
	dev-libs/boost
	x11-libs/pango
	dev-cpp/libxmlpp
	media-libs/glew
	media-libs/libsdl[joystick,opengl]
	media-libs/libpng
	media-libs/jpeg
	tools? ( media-gfx/imagemagick[png] )
	editor? ( media-gfx/imagemagick[png] )
	webcam? ( media-libs/opencv[v4l] )
	>=media-video/ffmpeg-0.4.9_p20070616-r20
	alsa? ( media-libs/alsa-lib )
	jack? ( media-sound/jack-audio-connection-kit )
	portaudio? ( media-libs/portaudio )
	gstreamer? ( media-libs/gstreamer )
	pulseaudio? ( media-sound/pulseaudio )
	sys-apps/help2man
	!games-arcade/ultrastar-ng"
# Waiting for portmidi to enter portage (#90614)
#RDEPEND="${RDEPEND}
#	midi? ( media-libs/portmidi )"
DEPEND="${RDEPEND}
    >=dev-util/cmake-2.6.0"

S="${WORKDIR}/${MY_P}"

src_unpack() {
	if [ "${PV}" != "9999" ]; then
		unpack "${MY_P}.tar.bz2"
	else
		git_src_unpack
		cd "${S}"
	fi
	if use songs; then
		cd "${S}"
		unpack "${SONGS_PN}-jc-1.zip"
		unpack "${SONGS_PN}-libre-3.zip"
		unpack "${SONGS_PN}-restricted-3.zip"
		unpack "${SONGS_PN}-shearer-1.zip"
		cd "${S}"
	fi
}

src_configure() {
	local mycmakeargs="
		$(cmake-utils_use alsa LibDA_PLUGIN_ALSA)
		$(cmake-utils_use jack LibDA_PLUGIN_JACK)
		$(cmake-utils_use gstreamer LibDA_PLUGIN_GSTREAMER)
		$(cmake-utils_use portaudio LibDA_PLUGIN_PORTAUDIO)
		$(cmake-utils_use pulseaudio LibDA_PLUGIN_PULSEAUDIO)
		$(cmake-utils_use_enable tools TOOLS)
		$(cmake-utils_use_enable editor EDITOR)
		-DCMAKE_INSTALL_PREFIX=${GAMES_PREFIX}
		-DSHARE_INSTALL=share/performous
		-DLOCALE_DIR=/usr/share
		-DLIBDA_AUTODETECT_PLUGINS=false
		-DLIBDA_PLUGIN_TESTING=false
		-DCMAKE_BUILD_TYPE=Release"

	cmake-utils_src_configure
}

src_compile() {
	cmake-utils_src_compile
}

src_install() {
	DOCS="docs/*.txt" cmake-utils_src_install

	if use songs; then
		insinto "/usr/share/games/ultrastar"
		doins -r "${S}/songs" || die "doins songs failed"
	fi
	doicon "${S}/data/${PN}.xpm"
	domenu "${S}/data/${PN}.desktop"
	prepgamesdirs
}
