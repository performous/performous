# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header$

inherit cvs games

DESCRIPTION="SingStar GPL clone"
HOMEPAGE="http://sourceforge.net/projects/ultrastar-ng/"
SRC_URI=""

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~x86 ~amd64"

IUSE="deprecated_cairo_svg novideo xine gstreamer opengl debug"

ECVS_SERVER="ultrastar-ng.cvs.sourceforge.net:/cvsroot/ultrastar-ng"
ECVS_MODULE="UltraStar-ng"
ECVS_AUTH="pserver"
ECVS_USER="anonymous"
ECVS_PASS=""

DEPEND="
	>=x11-libs/cairo-1.2
	xine? ( media-libs/xine-lib )
	gstreamer? ( >=media-libs/gstreamer-0.10 )
	!deprecated_cairo_svg? ( >=gnome-base/librsvg-2 )

	media-libs/libsdl
	media-libs/sdl-image
	media-libs/sdl-gfx
	!novideo? ( media-libs/smpeg )

	>sci-libs/fftw-3
	media-libs/alsa-lib
	
	sys-apps/help2man"

S=${WORKDIR}/${ECVS_MODULE}

pkg_setup() {
	if use deprecated_cairo_svg ; then
		ewarn "librsvg flag is not used, please be aware that"
		ewarn "compiling ultrastar-ng with the deprecated_cairo_svg use flag"
		ewarn "will compile the game with the old, deprecated, ugly"
		ewarn "cairo-svg rendering engine"
	fi
	if ! use xine && ! use gstreamer ; then
		eerror "You must choose either xine or gstreamer audio support"
	fi
	if use xine && use gstreamer ; then
		ewarn "Since both xine and gstreamer audio support has been"
		ewarn "enabled, only xine will be used as default"
	fi
	if use opengl && ! built_with_use media-libs/libsdl opengl; then
		eerror "opengl flag set, but libsdl wasn't build with opengl support"
	fi
}

src_compile() {
	cd "${S}"
	./autogen.sh
	local myconf=

	if use novideo ; then
		myconf="${myconf} --with-video=disable"
	else
		myconf="${myconf} --with-video=smpeg"
	fi

	if use deprecated_cairo_svg ; then
		myconf="${myconf} --with-svg=cairo"
	else
		myconf="${myconf} --with-svg=librsvg"
	fi

	if use xine ; then
		myconf="${myconf} --with-audio=xine"
	else
		myconf="${myconf} --with-audio=gstreamer"
	fi

	if use opengl ; then
		myconf="${myconf} --with-graphic-driver=opengl"
	else
		myconf="${myconf} --with-graphic-driver=sdl"
	fi
	if use debug ; then
		myconf="${myconf} --enable-debug"
	else
		myconf="${myconf} --disable-debug"
	fi

	egamesconf ${myconf} || die
	emake || die "emake failed"
}

src_install() {
	make DESTDIR="${D}" install || die "make install failed"
	doicon data/ultrastar-ng.xpm
	domenu data/ultrastar-ng.desktop
	prepgamesdirs
}
