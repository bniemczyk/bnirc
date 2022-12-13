aclocal 
automake -f --add-missing
libtoolize -c --force
autoconf
./configure $*
