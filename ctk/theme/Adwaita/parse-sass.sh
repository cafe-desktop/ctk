#! /bin/bash

if [ ! "$(which sassc 2> /dev/null)" ]; then
   echo sassc needs to be installed to generate the css.
   exit 1
fi

SASSC_OPT="-M -t compact"

echo Generating the css...

sassc $SASSC_OPT ctk-contained.scss ctk-contained.css
sassc $SASSC_OPT ctk-contained-dark.scss ctk-contained-dark.css
