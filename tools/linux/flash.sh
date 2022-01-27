#!/bin/sh

SCRIPTPATH=$(readlink -f "$0")
TOOLSPATH=$(dirname "$SCRIPTPATH")

echo $TOOLSPATH

# curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

$TOOLSPATH/arduino-cli upload --fqbn adafruit:samd:adafruit_feather_m0 
