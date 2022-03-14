#!/bin/sh

readlinkf(){ perl -MCwd -e 'print Cwd::abs_path shift' "$1";}

SCRIPTPATH=$(readlinkf "$0")
TOOLSPATH=$(dirname "$SCRIPTPATH")

echo $TOOLSPATH

# pip install pillow

# curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

$TOOLSPATH/arduino-cli config init --overwrite --additional-urls https://adafruit.github.io/arduino-board-index/package_adafruit_index.json

$TOOLSPATH/arduino-cli core update-index

$TOOLSPATH/arduino-cli core install arduino:samd 
$TOOLSPATH/arduino-cli core install adafruit:samd

$TOOLSPATH/arduino-cli lib update-index
$TOOLSPATH/arduino-cli lib install RTCZero "Adafruit GFX Library" "Adafruit BusIO" "Adafruit SPIFlash" "SdFat - Adafruit Fork" "Adafruit TinyUSB Library" "Adafruit Zero DMA Library" 

#arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0:usbstack=tinyusb,debug=on,opt=small pawos.ino