# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header$

inherit cvs games

DESCRIPTION="SingStar GPL clone"
HOMEPAGE="http://sourceforge.net/projects/ultrastar-ng/"
SRC_URI=""

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~x86"

IUSE="deprecated_cairo_svg novideo xine gstreamer"

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
	media-libs/alsa-lib"

S=${WORKDIR}/${ECVS_MODULE}

pkg_setup() {
	if use deprecated_cairo_svg ; then
		ewarn "librsvg flag is not used, please be aware that"
		ewarn "compiling ultrastar-ng without the librsvg use flag"
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

	egamesconf ${myconf} || die
	emake || die "emake failed"
}

src_install() {
	make DESTDIR="${D}" install || die "make install failed"
	prepgamesdirs
}
