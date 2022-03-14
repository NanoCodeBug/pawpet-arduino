
# flash
param(
    [string] $serialPort
)

if (-not $serialPort)
{
    $serialPort = Get-CimInstance -ClassName Win32_SerialPort -Filter "PNPDeviceID like 'USB\\VID_239A&PID_800B&MI_00%'" | Select-Object -ExpandProperty DeviceID
}

if (-not $serialPort)
{
    $serialPort = Get-CimInstance -ClassName Win32_SerialPort -Filter "PNPDeviceID like 'USB\\VID_239A&PID_0015&MI_00%'" | Select-Object -ExpandProperty DeviceID
}

if (-not $serialPort)
{
    $serialPort = Get-CimInstance -ClassName Win32_SerialPort -Filter "PNPDeviceID like 'USB\\VID_03EB&PID_2402&MI_00%'" | Select-Object -ExpandProperty DeviceID
}

if($serialPort)
{
    Write-Host "Using com port: " $serialPort
}
else 
{
    Write-Host "Failed to find com port, define manually as arguement."   
}

# 
$buildString = @(
                "upload",
                "--fqbn",
                "nanocodebug:samd:pawpet_m0",
                "pawos.ino",
                "-p",
                "$serialPort"
)

Invoke-Expression "$PSScriptRoot\arduino-cli.exe $($buildString -join ' ')"
