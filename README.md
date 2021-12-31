# Build Instructions

Arduino SDK, pin layout of board is identical to a Adafruit Feather M0.

## Linux Arduino SDK and Dependency Setup
tools/linux/sdk-setup.sh

<todo, linux build script>
arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0:usbstack=tinyusb,debug=on,opt=small pawos.ino

## Windows Arduino SDK and Dependency Setup
win/tools/sdk-setup.ps1

win/tools/build.ps1

# Flash Instructions

will be identical to arduino sdk for now

ideally use custom script for handing images to internal flash via msc and new firmware over UF2 format

TODO: look into using openocd for flashing bootloader

# Licensing 

## Code
All code is licensed under the MIT license.

## Assets
Graphics, art, and models found in `sprites` and `paw-case` are licensed under the Creative Commons Attribution-NonCommercial 4.0 International Public License.  