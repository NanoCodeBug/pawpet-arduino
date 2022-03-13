param(
    [switch]$release,
    [switch]$exportAseprite,
    [switch]$clean
)

$buildFolder="$PSScriptRoot\..\..\build"
$projectRoot="$PSScriptRoot\..\.."
$projectTools="$PSScriptRoot\..\..\tools"

$buildargs = @()

if ($release)
{
    # build args
    $buildargs = @(
        "usbstack=tinyusb",
        "debug=off",
        "opt=small"
    )
}
else
{
    $buildargs = @(
        "usbstack=tinyusb",
        "debug=on",
        "opt=small"
    )
}



#create build folders
New-Item -ItemType Directory -Force -Path "$buildFolder\assets"

if ($exportAseprite) {
    # convert aseprites to png
    Invoke-Expression "$PSScriptRoot\aseprite-export.ps1" 
}

# convert png to c header
Remove-Item -Force -Recurse -Path "$buildFolder\assets\*"
Invoke-Expression "python $projectRoot\png2c\png2c.py $projectRoot\sprites\ $projectRoot\src\graphics\sprites.h $buildFolder\assets\" 

$buildString = @( 
    "compile",
    "--fqbn",
    "adafruit:samd:adafruit_feather_m0:$($buildargs -join ',')",
    "--build-property build.extra_flags+=`"-DCRYSTALLESS=1 -D__SAMD21G18A__ -DADAFRUIT_FEATHER_M0 -DARDUINO_SAMD_ZERO -DARM_MATH_CM0PLUS -DUSB_PRODUCT=PawPet_M0 {build.usb_flags}`"",
    "pawos.ino",
    "--output-dir",
    "build" 
)

if ($clean)
{
    $buildString += "--clean"
}

Write-Host $buildString

Invoke-Expression "$PSScriptRoot\arduino-cli.exe $($buildString -join ' ')" -Verbose