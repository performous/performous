#!/bin/sh
# cleanup
rm -f config.cache
rm -f config.log
rm -f config.status
rm -f config.h*
rm -f aclocal.m4
rm -rf autom4te.cache
rm -rf m4

ACLOCAL=`which aclocal 2>/dev/null`
AUTOHEADER=`which autoheader 2>/dev/null`
AUTOCONF=`which autoconf 2>/dev/null`
LIBTOOLIZE=`which libtoolize 2>/dev/null || which glibtoolize 2>/dev/null`
AUTOMAKE=`which automake 2>/dev/null`

if test "x$ACLOCAL" = "x" ; then
  echo "Cannot find aclocal tool"
  exit 1
fi
if test "x$AUTOHEADER" = "x" ; then
  echo "Cannot find autoheader tool"
  exit 1
fi
if test "x$AUTOCONF" = "x" ; then
  echo "Cannot find autoconf tool"
  exit 1
fi
if test "x$LIBTOOLIZE" = "x" ; then
  echo "Cannot find libtoolize compliant tool (libtoolize or glibtoolize)"
  exit 1
fi
if test "x$AUTOMAKE" = "x" ; then
  echo "Cannot find automake tool"
  exit 1
fi

echo "### Running alocal"
$ACLOCAL
echo "### Running autoheader"
touch config.h.incl
$AUTOHEADER
test -f "config.h.in:config.h.incl" && mv "config.h.in:config.h.incl" config.h.in
echo "### Running autoconf"
$AUTOCONF
echo "### Running libtoolize"
$LIBTOOLIZE --force
echo "### Running automake"
$AUTOMAKE -a
