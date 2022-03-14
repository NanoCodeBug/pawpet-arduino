#!/bin/sh

SCRIPTPATH=$(readlink -f "$0")
TOOLSPATH=$(dirname "$SCRIPTPATH")

echo $TOOLSPATH

# curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

$TOOLSPATH/arduino-cli upload --fqbn nanocodebug:samd:pawpet_m0 pawos.ino /dev/ttyACM0
