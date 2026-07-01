$ErrorActionPreference = 'Stop'

$shell = Get-Content -Raw (Join-Path $PSScriptRoot '..\shell.html')

function Expect-Contains($Text, $Needle, $Message) {
    if (-not $Text.Contains($Needle)) {
        Write-Error $Message
    }
}

Expect-Contains $shell 'recenterDesk(Math.PI / 2, 0.5, 7.0);' 'lab desk should face the seated character'
Expect-Contains $shell 'function faceGlass() {' 'lab scene should centralize the glass-facing character rotation'
Expect-Contains $shell 'figureRoot.rotation.y = Math.PI;' 'lab character should face -Z before walking toward the glass'
Expect-Contains $shell 'faceGlass();' 'lab walk phase should set facing before starting the walking animation'

Write-Output 'Scene layout tests passed.'
