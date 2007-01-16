# cleanup
rm -f config.cache
rm -f config.log
rm -f config.status
rm -f config.h*
rm -f aclocal.m4
rm -rf autom4te.cache

echo "### Running alocal"
aclocal
echo "### Running autoheader"
touch config.h.incl
autoheader
test -f "config.h.in:config.h.incl" && mv "config.h.in:config.h.incl" config.h.in
echo "### Running autoconf"
autoconf
echo "### Running automake"
automake -a
