#!/bin/bash

SRCDIR="/$HOME/Arduino/libraries/"
cd "$(dirname $0)"
rsync -rva $SRCDIR/rgBtn  $SRCDIR/rgCsv  $SRCDIR/rgRng .
