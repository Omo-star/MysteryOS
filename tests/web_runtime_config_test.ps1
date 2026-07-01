$ErrorActionPreference = 'Stop'

$cmake = Get-Content -Raw (Join-Path $PSScriptRoot '..\CMakeLists.txt')

function Expect-Contains($Text, $Needle, $Message) {
    if (-not $Text.Contains($Needle)) {
        Write-Error $Message
    }
}

function Expect-NotContains($Text, $Needle, $Message) {
    if ($Text.Contains($Needle)) {
        Write-Error $Message
    }
}

Expect-NotContains $cmake '-sALLOW_MEMORY_GROWTH=1' 'web build should not use growable Wasm memory because TextDecoder rejects resizable ArrayBuffers'
Expect-Contains $cmake '-sINITIAL_MEMORY=256MB' 'web build should reserve a fixed heap large enough for embedded assets'

Write-Output 'Web runtime config tests passed.'
