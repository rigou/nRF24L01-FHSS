#!/bin/bash

SRCDIR="/$HOME/Projects/Arduino/libraries"
cd "$(dirname $0)"
rsync -rva $SRCDIR/rgBtn  $SRCDIR/rgCsv  $SRCDIR/rgDebug  $SRCDIR/rgRng  $SRCDIR/rgStr .
