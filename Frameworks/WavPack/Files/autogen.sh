#!/bin/sh

touch NEWS README AUTHORS ChangeLog
aclocal
libtoolize --copy
automake --add-missing
autoconf
