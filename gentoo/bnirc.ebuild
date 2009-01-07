# Copyright 2008 bnIRC
# Distributed under the terms of the GNU General Public License v2

inherit  eutils
DESCRIPTION="A modular textUI IRC client with IPv6 support"
HOMEPAGE="http://sf.net/projects/bnirc"
SRC_URI="http://superb-east.dl.sourceforge.net/sourceforge/bnirc/bnIRC-${PV}.tar.gz"
LICENSE="GPL-2"
SLOT="0"
KEYWORDS="alpha amd64 arm hppa ia64 mips ppc ppc64 s390 sh sparc x86 ~x86-fbsd"
IUSE=""

RDEPEND="sys-libs/ncurses
	python? ( dev-lang/python )"
DEPEND="${RDEPEND}
	>=dev-util/pkgconfig-0.9.0"
RDEPEND="${RDEPEND}
	!net-irc/bnirc-svn"

src_unpack() {
	unpack ${A}
	cd "${S}"

	epunt_cxx

}

src_compile() {
	econf \
		--with-ncurses \
		|| die "econf failed"
	emake || die "emake failed"
}

src_install() {
	make \
		DESTDIR="${D}" \
		docdir=/usr/share/doc/${PF} \
		install || die "make install failed"
	dodoc AUTHORS  README TODO NEWS || die "dodoc failed"
}
