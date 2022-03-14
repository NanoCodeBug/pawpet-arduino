#!/bin/sh

readlinkf(){ perl -MCwd -e 'print Cwd::abs_path shift' "$1";}

SCRIPTPATH=$(readlinkf "$0")
TOOLSPATH=$(dirname "$SCRIPTPATH")

echo $TOOLSPATH

# pip install pillow

# curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

$TOOLSPATH/arduino-cli config init --overwrite --additional-urls "https://adafruit.github.io/arduino-board-index/package_adafruit_index.json,https://raw.githubusercontent.com/NanoCodeBug/pawpet/main/board/package_pawpet_index.json"

$TOOLSPATH/arduino-cli core update-index

$TOOLSPATH/arduino-cli core install arduino:samd 
$TOOLSPATH/arduino-cli core install adafruit:samd
$TOOLSPATH/arduino-cli core install nanocodebug:samd

$TOOLSPATH/arduino-cli lib update-index
$TOOLSPATH/arduino-cli lib install RTCZero "Adafruit GFX Library" "Adafruit BusIO" "Adafruit SPIFlash" "SdFat - Adafruit Fork" "Adafruit Zero DMA Library" 
