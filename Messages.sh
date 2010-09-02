#! /bin/sh
$EXTRACTRC *.rc *.ui >> rc.cpp
$XGETTEXT `find . -name '*.cpp'` -o $podir/kmix.pot
rm -f rc.cpp
