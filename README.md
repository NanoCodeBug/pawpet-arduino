# Build Instructions

Arduino SDK, pin layout of board is identical to a Adafruit Feather M0. 

## Linux Arduino SDK and Dependency Setup
tools/linux/sdk-setup.sh

tools/linux/build.sh

## Windows Arduino SDK and Dependency Setup
tools/win/sdk-setup.ps1

tools/win/build.ps1

## Mac Arduino SDK and Dependency Setup
tools/osx/sdk-setup.ps1

tools/osx/build.ps1

# Bootloader Instructions

1. flash adafruit bootloader https://github.com/adafruit/uf2-samdx1
2. set boot brotection fuses to 8kb (bootprot fuse value 0x2)
3. (optional) flash `pawos_setup.hex`
4. disable BOD33 fuse via `pawos_setup.hex` or preffered programmer device

# Flash Instructions

1. connect device via usb
2. double click reset button to enter bootloader
3. flash pawos via arduino-cli (`tools/<plat>/flash.sh`)

# Update Instructions
1. wake PawPet from sleep
2. connect via USB
3. flash pawos via arduino-cli (`tools/<plat>/flash.sh`)

TODO: update instructions that use UF2 instead, should be possible to use a script to put the device into bootloader mode, and then copy the file over via system file browser, or dynamically detect the device attach via usb id and drive name.

TODO: consider a self packaged C# exe or python script?

TODO: look into using openocd for flashing bootloader
TODO: fork adafruit bootloader to have low power operation enabled by default, bootloader blocks startup below 2.2v right now. in theory device can operate to 1.9v before display cuts out


# Simulator
SDL2 and Webasm wrappers.

`tools/win/build.ps1 -simulator`

builds a SDL2 exe version of the pawpet

`tools/win/build.ps1 -webSimulator`

builds a webasm version of the pawpet

# Licensing 

## Code
All code is licensed under the MIT license.

## Assets
Graphics, art, and models found in `sprites` and `paw-case` are licensed under the Creative Commons Attribution-NonCommercial 4.0 International Public License.  