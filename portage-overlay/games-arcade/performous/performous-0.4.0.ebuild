# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /cvsroot/ultrastar-ng/UltraStar-ng/portage-overlay/games-arcade/performous/performous-9999.ebuild,v 1.10 2007/09/29 13:04:19 yoda-jm Exp $

EAPI=2
inherit cmake-utils games

RESTRICT="nostrip"

SONGS_PN=ultrastar-songs
MY_PN=Performous
MY_P=${MY_PN}-${PV}-Source
SONGS_PN=ultrastar-songs

DESCRIPTION="Karaoke game similar to Singstar"
HOMEPAGE="http://performous.org"
SRC_URI="mirror://sourceforge/${PN}/${MY_P}.tar.bz2
	songs? ( 
		mirror://sourceforge/${PN}/${SONGS_PN}-jc-1.zip
		mirror://sourceforge/${PN}/${SONGS_PN}-libre-3.zip
		mirror://sourceforge/${PN}/${SONGS_PN}-restricted-3.zip
		mirror://sourceforge/${PN}/${SONGS_PN}-shearer-1.zip
	)"

EGIT_REPO_URI="git://performous.git.sourceforge.net/gitroot/performous/performous"

LICENSE="GPL-2
	songs? (
		CCPL-Attribution-ShareAlike-NonCommercial-2.5
		CCPL-Attribution-NonCommercial-NoDerivs-2.5
	)"
SLOT="0"
KEYWORDS="~x86 ~amd64 ~ppc ~ppc64"

IUSE="debug alsa portaudio pulseaudio jack songs gstreamer tools editor"

RDEPEND="gnome-base/librsvg
	>=dev-libs/boost-1.36.0
	x11-libs/pango
	dev-cpp/libxmlpp
	media-libs/glew
	media-libs/libsdl[joystick,opengl]
	media-gfx/imagemagick[png]
	>=media-video/ffmpeg-0.4.9_p20070616-r20
	alsa? ( media-libs/alsa-lib )
	jack? ( media-sound/jack-audio-connection-kit )
	portaudio? ( media-libs/portaudio )
	gstreamer? ( media-libs/gstreamer )
	pulseaudio? ( media-sound/pulseaudio )
	sys-apps/help2man
	!games-arcade/ultrastar-ng"
DEPEND="${RDEPEND}
    >=dev-util/cmake-2.6.0"

S="${WORKDIR}/${MY_P}"

src_unpack() {
	unpack "${MY_P}.tar.bz2"
	if use songs; then
		cd "${S}"
		unpack "${SONGS_PN}-jc-1.zip"
		unpack "${SONGS_PN}-libre-3.zip"
		unpack "${SONGS_PN}-restricted-3.zip"
		unpack "${SONGS_PN}-shearer-1.zip"
	fi
}

src_configure() {
	local mycmakeargs="
		$(cmake-utils_use alsa LIBDA_PLUGIN_ALSA)
		$(cmake-utils_use jack LIBDA_PLUGIN_JACK)
		$(cmake-utils_use gstreamer LIBDA_PLUGIN_GSTREAMER)
		$(cmake-utils_use portaudio LIBDA_PLUGIN_PORTAUDIO)
		$(cmake-utils_use pulseaudio LIBDA_PLUGIN_PULSEAUDIO)
		$(cmake-utils_use_enable tools TOOLS)
		$(cmake-utils_use_enable editor EDITOR)
		-DCMAKE_INSTALL_PREFIX=${GAMES_PREFIX}
		-DSHARE_INSTALL=share/performous/
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
