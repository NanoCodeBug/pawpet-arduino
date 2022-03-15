
param(
    [string] $jlinkpath = "C:\Program Files\SEGGER\JLink\JLink.exe",
    [switch] $bootloader,
    [switch] $setup,
    [switch] $os
)

# $buildFolder="$PSScriptRoot\..\..\build"
# $projectRoot="$PSScriptRoot\..\.."
$projectTools="$PSScriptRoot\..\..\tools"

if($bootloader)
{
    Start-Process -FilePath $jlinkpath -ArgumentList "-device AT91SAMD21G18 -if swd -speed 4000 -autoconnect 1 -CommandFile $projectTools/bootloader.jlink" -NoNewWindow -Wait
}

if($setup)
{
    Start-Process -FilePath $jlinkpath -ArgumentList "-device AT91SAMD21G18 -if swd -speed 4000 -autoconnect 1 -CommandFile $projectTools/setup.jlink" -NoNewWindow -Wait
}

if($os)
{
    Start-Process -FilePath $jlinkpath -ArgumentList "-device AT91SAMD21G18 -if swd -speed 4000 -autoconnect 1 -CommandFile $projectTools/os.jlink" -NoNewWindow -Wait
}

Write-Host "done"