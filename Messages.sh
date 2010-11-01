#! /bin/sh
$EXTRACTRC *.rc *.ui >> rc.cpp
$XGETTEXT `find . -name '*.cpp' | grep -v '/tests/'` -o $podir/kmix.pot
rm -f rc.cpp
