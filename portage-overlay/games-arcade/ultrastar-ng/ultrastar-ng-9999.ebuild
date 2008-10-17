# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /cvsroot/ultrastar-ng/UltraStar-ng/portage-overlay/games-arcade/ultrastar-ng/ultrastar-ng-9999.ebuild,v 1.10 2007/09/29 13:04:19 yoda-jm Exp $

inherit games subversion

RESTRICT="nostrip"

SONGS_PN=ultrastar-songs
SONGS_P=${SONGS_PN}-2

DESCRIPTION="SingStar GPL clone"
HOMEPAGE="http://sourceforge.net/projects/ultrastar-ng/"
SRC_URI="songs? ( mirror://sourceforge/${PN}/${SONGS_P}.tar.bz2 )"

ESVN_REPO_URI="https://ultrastar-ng.svn.sourceforge.net/svnroot/ultrastar-ng/trunk"
ESVN_PROJECT="UltraStar-NG"

LICENSE="GPL-2
	songs? (
		CCPL-Attribution-ShareAlike-NonCommercial-2.5
		CCPL-Attribution-NonCommercial-NoDerivs-2.5
	)"
SLOT="0"
KEYWORDS="~x86 ~amd64"

IUSE="debug alsa portaudio pulseaudio jack songs"

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
	sys-apps/help2man"
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

src_unpack() {
	subversion_src_unpack
	if use songs; then
		unpack "${SONGS_P}.tar.bz2"
	fi
}

src_compile() {
	cmake \
		-DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
		-DCMAKE_INSTALL_PREFIX="${GAMES_PREFIX}" \
		-DDATA_INSTALL_DIR="${GAMES_DATADIR}/${PN}" \
		. || die "cmake failed"
	emake || die "emake failed"
}

src_install() {
	emake DESTDIR="${D}" install || die "make install failed"
	keepdir "${GAMES_DATADIR}"/ultrastar/songs
	if use songs; then
		insinto "${GAMES_DATADIR}"/ultrastar
		doins -r songs || die "doins songs failed"
	fi
	mv "${D}${GAMES_DATADIR}/${PN}"/{applications,pixmaps} "${D}"/usr/share/
	dodoc AUTHORS ChangeLog README TODO
	prepgamesdirs
}
