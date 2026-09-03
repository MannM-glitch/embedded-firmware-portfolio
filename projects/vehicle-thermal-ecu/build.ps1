$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectRoot "build"
$SourceDir = Join-Path $ProjectRoot "src"
$TestDir = Join-Path $ProjectRoot "tests"
$MsysUcrtBin = Join-Path $env:USERPROFILE "msys64\ucrt64\bin"

if ((Test-Path $MsysUcrtBin) -and -not (($env:Path -split ';') -contains $MsysUcrtBin)) {
    $env:Path = "$env:Path;$MsysUcrtBin"
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$Gcc = Get-Command gcc -ErrorAction SilentlyContinue
$Clang = Get-Command clang -ErrorAction SilentlyContinue

if ($Gcc) {
    $Compiler = $Gcc.Source
}
elseif ($Clang) {
    $Compiler = $Clang.Source
}
else {
    Write-Host "No GCC or Clang compiler found on PATH."
    exit 1
}

$CommonFlags = @("-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I$SourceDir")
$CanSource = Join-Path $SourceDir "can_bus_sim.c"
$EcuSource = Join-Path $SourceDir "thermal_ecu.c"
$MainSource = Join-Path $SourceDir "main.c"
$TestSource = Join-Path $TestDir "test_thermal_ecu.c"

& $Compiler @CommonFlags $CanSource $EcuSource $MainSource "-o" (Join-Path $BuildDir "vehicle-thermal-ecu.exe")
& $Compiler @CommonFlags $CanSource $EcuSource $TestSource "-o" (Join-Path $BuildDir "thermal-ecu-tests.exe")

Write-Host "Built: $(Join-Path $BuildDir 'vehicle-thermal-ecu.exe')"
Write-Host "Built: $(Join-Path $BuildDir 'thermal-ecu-tests.exe')"
