# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /cvsroot/ultrastar-ng/UltraStar-ng/portage-overlay/games-arcade/performous/performous-9999.ebuild,v 1.10 2007/09/29 13:04:19 yoda-jm Exp $

inherit games cmake-utils

RESTRICT="nostrip"

MY_PN=Performous
MY_P=${MY_PN}-${PV}-Source
SONGS_PN=ultrastar-songs
SONGS_P=${SONGS_PN}-2

DESCRIPTION="SingStar GPL clone"
HOMEPAGE="http://performous.org"
SRC_URI=" mirror://sourceforge/${PN}/${MY_P}.tar.bz2
	songs? ( mirror://sourceforge/ultrastar-ng/${SONGS_P}.tar.bz2 )"

LICENSE="GPL-2
	songs? (
		CCPL-Attribution-ShareAlike-NonCommercial-2.5
		CCPL-Attribution-NonCommercial-NoDerivs-2.5
	)"
SLOT="0"
KEYWORDS="~x86 ~amd64 ~ppc ~ppc64"

IUSE="debug alsa portaudio pulseaudio jack songs gstreamer"

RDEPEND="gnome-base/librsvg
	dev-libs/boost
	x11-libs/pango
	dev-cpp/libxmlpp
	media-libs/libsdl
	media-gfx/imagemagick
	(
		virtual/opengl
		virtual/glu
	)
	>=media-video/ffmpeg-0.4.9_p20070616-r20
	alsa? ( media-libs/alsa-lib )
	jack? ( >=media-sound/jack-audio-connection-kit )
	portaudio? ( media-libs/portaudio )
	gstreamer? ( media-libs/gstreamer )
	pulseaudio? ( media-sound/pulseaudio )
	sys-apps/help2man
	!games-arcade/ultrastar-ng"
DEPEND="${RDEPEND}
    >=dev-util/cmake-2.6.0"

pkg_setup() {
	games_pkg_setup
	if ! built_with_use media-libs/libsdl opengl; then
		eerror "libsdl wasn't build with opengl support"
	fi
	if ! built_with_use --missing true dev-libs/boost threads ; then
		eerror "Please emerge dev-libs/boost with USE=threads"
	fi
}

src_compile() {
	cd "${MY_P}"
	mkdir build
	cd build
	cmake \
		-DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
		-DCMAKE_INSTALL_PREFIX="${GAMES_PREFIX}" \
		$(cmake-utils_use_with alsa ALSA) \
		$(cmake-utils_use_with jack JACK) \
		$(cmake-utils_use_with pulseaudio PULSEAUDIO) \
		$(cmake-utils_use_with portaudio PORTAUDIO) \
		$(cmake-utils_use_with gstreamer GSTREAMER) \
		.. || die "cmake failed"
	emake || die "emake failed"
}

src_install() {
	cd "${MY_P}/build"
	emake DESTDIR="${D}" install || die "make install failed"
	keepdir "${GAMES_DATADIR}"/ultrastar/songs
	if use songs; then
		insinto "${GAMES_DATADIR}"/ultrastar
		doins -r ../../songs || die "doins songs failed"
	fi
	rm -rf "${D}${GAMES_PREFIX}"/share/"${PN}"/{applications,pixmaps}
	doicon data/${PN}.xpm
	domenu data/${PN}.desktop
	dodoc ../docs/*.txt
	prepgamesdirs
}
