# Copyright 1999-2007 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

inherit eutils games

MY_PN=UltraStar-ng
MY_P=${MY_PN}-${PV}

DESCRIPTION="SingStar GPL clone"
HOMEPAGE="http://sourceforge.net/projects/ultrastar-ng/"
SRC_URI="mirror://sourceforge/${PN}/${MY_P}.tar.gz"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE="novideo opengl xine debug alsa gstreamer portaudio"

RDEPEND="gnome-base/librsvg
	dev-libs/boost
	x11-libs/pango
	media-libs/sdl-image
	media-libs/sdl-gfx
	xine? ( media-libs/xine-lib )
	!xine? ( media-libs/gstreamer )
	opengl? (
		virtual/opengl
		virtual/glu
	)
	alsa? ( media-libs/alsa-lib )
	portaudio? ( media-libs/portaudio )
	gstreamer? ( >=media-libs/gstreamer-0.10 )
	!novideo? ( media-libs/smpeg )"
DEPEND="${RDEPEND}
	dev-util/pkgconfig"

S=${WORKDIR}/${MY_P}

pkg_setup() {
	games_pkg_setup
	if use opengl && ! built_with_use media-libs/libsdl opengl; then
		eerror "opengl flag set, but libsdl wasn't build with opengl support"
	fi
}

src_compile() {
	local myconf

	if use novideo; then
		myconf="--with-video=disable"
	else
		myconf="--with-video=smpeg"
	fi
	if use opengl; then
		myconf="$myconf --with-graphic-driver=opengl"
	else
		myconf="$myconf --with-graphic-driver=sdl"
	fi
	if use xine; then
		myconf="$myconf --with-audio=xine"
	else
		myconf="$myconf --with-audio=gstreamer"
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
	emake DESTDIR="${D}" install || die "emake install failed"
	keepdir "${GAMES_DATADIR}"/${PN}/songs
	mv "${D}${GAMES_DATADIR}"/{applications,pixmaps} "${D}"/usr/share/
	dodoc AUTHORS ChangeLog README TODO
	prepgamesdirs
}
