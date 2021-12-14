
$aseprite = "C:\Program Files\Aseprite\Aseprite.exe"

$projectRoot="$PSScriptRoot\..\.."
$spritePath = "$projectRoot\..\sprites"
$destPath = "$projectRoot\sprites"

$slices = @(
    # ,("maze-tiles-large", "maze-tile")
)

$images = @(
    ,("battery3", "battery_16x8")
    ,("icons8", "icons_8x8")
    ,("sleeptest1", "sleeptest_64x64")
)

$animations = @(
    ,("pet1-sit", "pet_sit_64x64")
)
# export slices of part file
foreach ($file in $slices) {
    Write-Host "slicing $($file)"
	& $aseprite -b "$($spritePath)\$($file[0]).aseprite" --save-as "$($destPath)\$($file[1])_`{slice`}.png"
}

# export animation data and png of other files
foreach ($file in $animations) {
    Write-Host "animation $($file)"
    # --format json-array --data "$($destPath)\$($file[1]).json"
	& $aseprite -b "$($spritePath)\$($file[0]).aseprite" --list-tags --sheet "$($destPath)\$($file[1]).png"
}

# export image data only
foreach ($file in $images) {
    Write-Host "image $($file)"
	& $aseprite -b "$($spritePath)\$($file[0]).aseprite" --save-as "$($destPath)\$($file[1]).png" 
}
