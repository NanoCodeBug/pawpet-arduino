
# flash
param(
    [string] $serialPort
)

$buildString = @(
                "upload",
                "--fqbn",
                "adafruit:samd:adafruit_feather_m0",
                "pawos.ino",
                "-p",
                "$serialPort"
)

Invoke-Expression "$PSScriptRoot\arduino-cli.exe $($buildString -join ' ')"
