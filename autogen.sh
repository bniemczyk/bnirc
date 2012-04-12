aclocal-1.10 && automake-1.10 -a -c 
libtoolize -c --force
autoconf
./configure $*
