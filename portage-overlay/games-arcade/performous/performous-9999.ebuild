# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /cvsroot/ultrastar-ng/UltraStar-ng/portage-overlay/games-arcade/performous/performous-9999.ebuild,v 1.10 2007/09/29 13:04:19 yoda-jm Exp $

EAPI=2
inherit git games cmake-utils

RESTRICT="nostrip"

SONGS_PN=ultrastar-songs

DESCRIPTION="Karaoke game similar to Singstar"
HOMEPAGE="http://performous.org"
SRC_URI="songs? ( 
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

IUSE="debug alsa portaudio pulseaudio jack songs gstreamer tools"

RDEPEND="gnome-base/librsvg
	dev-libs/boost
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

pkg_setup() {
	if ! built_with_use --missing true dev-libs/boost threads ; then
		eerror "Please emerge dev-libs/boost with USE=threads"
	fi
}

src_unpack() {
	git_src_unpack
	if use songs; then
		cd "${S}"
		unpack "${SONGS_PN}-jc-1.zip"
		unpack "${SONGS_PN}-libre-3.zip"
		unpack "${SONGS_PN}-restricted-3.zip"
		unpack "${SONGS_PN}-shearer-1.zip"
		pwd
	fi
}

src_compile() {
	local mycmakeargs="
		$(cmake-utils_use_enable alsa LIBDA_PLUGIN_ALSA)
		$(cmake-utils_use_enable jack LIBDA_PLUGIN_ALSA)
		$(cmake-utils_use_enable gstreamer LIBDA_PLUGIN_GSTREAMER)
		$(cmake-utils_use_enable portaudio LIBDA_PLUGIN_PORTAUDIO)
		$(cmake-utils_use_enable pulseaudio LIBDA_PLUGIN_PULSEAUDIO)
		$(cmake-utils_use_enable tools TOOLS)
		-DCMAKE_INSTALL_PREFIX=${GAMES_PREFIX}
		-DSHARE_INSTALL=share/
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

	if test -f "${D}/usr/bin/performous.sh" ; then
		perl -pi -e "s+bin/performous+bin/performous.bin+" "${D}/usr/bin/performous.sh"
		mv "${D}/usr/bin/performous" "${D}/usr/bin/performous.bin"
		mv "${D}/usr/bin/performous.sh" "${D}/usr/bin/performous"
	fi

	if use songs; then
		insinto "/usr/share/games/ultrastar"
		doins -r "${S}/songs" || die "doins songs failed"
	fi
	doicon "${S}/data/${PN}.xpm"
	domenu "${S}/data/${PN}.desktop"
	prepgamesdirs
}
