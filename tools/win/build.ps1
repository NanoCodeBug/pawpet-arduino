param(
    [switch]$release,
    [switch]$exportAseprite,
    [switch]$clean,
    [switch]$simulator,
    [switch]$webSimulator,
    [switch]$firmware
)

$buildFolder="$PSScriptRoot\..\..\build"
$projectRoot="$PSScriptRoot\..\.."
$projectTools="$PSScriptRoot\..\..\tools"

$cmakePath = 'C:\Program` Files\CMake\bin\cmake.exe'
$emsdkPath = "C:\Users\nano\dev\emsdk"

$buildargs = @()

if ($release)
{
    # build args
    $buildargs = @(
        # "usbstack=tinyusb",
        "debug=off",
        "opt=small"
    )
}
else
{
    $buildargs = @(
        # "usbstack=tinyusb",
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

if ($firmware -or (($simulator -eq $False) -and ($webSimulator -eq $False)))
{
    $buildString = @( 
        "compile",
        "--fqbn",
        "nanocodebug:samd:pawpet_m0:$($buildargs -join ',')",
        # "--build-property build.extra_flags=`"-DCRYSTALLESS=1 -D__SAMD21G18A__ -DADAFRUIT_FEATHER_M0 -DARDUINO_SAMD_ZERO -DARM_MATH_CM0PLUS {build.usb_flags}`"",
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
}

if ($webSimulator) 
{
    if ($clean)
    {
        Remove-Item -Recurse -Path "$buildFolder\html" -Force -ErrorAction Ignore
    }

    New-Item -Path "$buildFolder\html" -ItemType Directory -Force -ErrorAction Ignore

    $cmakeArgs = @(
        "-S$projectRoot\simulator", 
        "-B$buildFolder\html", 
        "-G Ninja", 
        "-DCMAKE_TOOLCHAIN_FILE=$emsdkPath\upstream\emscripten\cmake\Modules\Platform\Emscripten.cmake",
        "-DCMAKE_BUILD_TYPE=Release"
        )

    $Env:SDL2_DIR="$projectRoot\simulator\SDL2"

    Invoke-Expression "$cmakePath $cmakeArgs"
    Invoke-Expression "ninja.exe -C $buildFolder\html"
}

if ($simulator)
{
    if ($clean)
    {
        Remove-Item -Recurse -Path "$buildFolder\win32" -Force -ErrorAction Ignore
    }

    New-Item -Path "$buildFolder\win32" -ItemType Directory -Force -ErrorAction Ignore
    
    $msvc = & 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe' -property installationpath -version 16.0
    
    Import-Module (Join-Path $msvc "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
    
    Enter-VsDevShell -VsInstallPath $msvc -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -no_logo'
    
    $cmakeArgs = @(
        "-S$projectRoot\simulator", 
        "-B$buildFolder\win32", 
        "-G Ninja", 
        "-DCMAKE_BUILD_TYPE=Debug"
        )
    
    $Env:SDL2_DIR="$projectRoot\simulator\SDL2"
    
    # Start-Process -FilePath "$cmakePath" -ArgumentList $cmakeArgs -NoNewWindow -Wait
    Invoke-Expression "$cmakePath $cmakeArgs"
    Invoke-Expression "ninja.exe -C $buildFolder\win32"
}


