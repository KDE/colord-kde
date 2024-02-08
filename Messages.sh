#! /usr/bin/env bash
$EXTRACTRC `find . -name "*.rc" -o -name "*.ui"` >> rc.cpp
$XGETTEXT `find . -name \*.cpp -o -name \*.qml` -o $podir/colord-kde.pot
rm -f rc.cpp

