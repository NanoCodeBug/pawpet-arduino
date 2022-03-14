
# python3 installed already

Invoke-Expression "pip install pillow"

# assume arduino cli exe present
$arduinoCli = "$PSScriptRoot/arduino-cli.exe"

Invoke-Expression "$arduinoCli config init --overwrite --additional-urls `"https://adafruit.github.io/arduino-board-index/package_adafruit_index.json,https://raw.githubusercontent.com/NanoCodeBug/pawpet/main/board/package_pawpet_index.json`""
Invoke-Expression "$arduinoCli core update-index"

Invoke-Expression "$arduinoCli core install arduino:samd"
Invoke-Expression "$arduinoCli core install adafruit:samd"
Invoke-Expression "$arduinoCli core install nanocodebug:samd"

Invoke-Expression "$arduinoCli lib install RTCZero  `"Adafruit GFX Library`" `"Adafruit BusIO`" `"Adafruit SPIFlash`" `"SdFat - Adafruit Fork`" `"Adafruit Zero DMA Library`"" 

Invoke-Expression "$arduinoCli lib install ZeroRegs"

# not needed, built in?
# `"Adafruit TinyUSB Library`"


# VSCODE
# c++ extension - intellisense, problem matching on build task
# cortex-debug - debugging over BMP or J-LINK??

# BUILDING
#arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0:usbstack=tinyusb,debug=on,opt=small pawos.ino

# FLASHING
# replace com port with whichever port device is assigned
# arduino-cli upload -p COM4 --fqbn adafruit:samd:adafruit_feather_m0 pawos.ino

# DEBUGGING
