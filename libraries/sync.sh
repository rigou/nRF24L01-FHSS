#!/bin/bash

SRCDIR="/$HOME/Arduino/libraries/"
cd "$(dirname $0)"
rsync -rva $SRCDIR/rgButton  $SRCDIR/rgCsv  $SRCDIR/rgRng .
