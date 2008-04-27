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

IUSE="ffmpeg xine gstreamer debug alsa portaudio songs"

RDEPEND="gnome-base/librsvg
	dev-libs/boost
	x11-libs/pango
	media-libs/libsdl
	media-gfx/imagemagick
	xine? ( media-libs/xine-lib )
	!xine? ( media-libs/gstreamer )
	(
		virtual/opengl
		virtual/glu
	)
	ffmpeg? ( media-video/ffmpeg )
	alsa? ( media-libs/alsa-lib )
	portaudio? ( media-libs/portaudio )
	gstreamer? ( >=media-libs/gstreamer-0.10 )
	sys-apps/help2man"
DEPEND="${RDEPEND}
    dev-util/pkgconfig"

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
	./autogen.sh
	local myconf

	if use ffmpeg ; then
		myconf="${myconf} --with-video=ffmpeg"
	else
		myconf="${myconf} --with-video=disable"
	fi

	if use xine ; then
		myconf="${myconf} --with-audio=xine"
	else
		myconf="${myconf} --with-audio=gstreamer"
	fi

	egamesconf \
		${myconf} \
		$(use_enable debug) \
		$(use_enable portaudio record-portaudio) \
		$(use_enable gstreamer record-gst) \
		$(use_enable alsa record-alsa) \
		|| die

	emake || die "emake failed"
}

src_install() {
	emake DESTDIR="${D}" install || die "make install failed"
	keepdir "${GAMES_DATADIR}"/${PN}/songs
	if use songs; then
		insinto "${GAMES_DATADIR}"/${PN}
		doins -r songs || die "doins songs failed"
	fi
	mv "${D}${GAMES_DATADIR}"/{applications,pixmaps} "${D}"/usr/share/
	dodoc AUTHORS ChangeLog README TODO
	prepgamesdirs
}
