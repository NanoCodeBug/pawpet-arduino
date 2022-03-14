#!/bin/sh

SCRIPTPATH=$(readlink -f "$0")
TOOLSPATH=$(dirname "$SCRIPTPATH")

echo $TOOLSPATH

# curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

$TOOLSPATH/arduino-cli compile --fqbn nanocodebug:samd:pawpet_m0:debug=on,opt=small pawos.ino
