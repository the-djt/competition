param(
    [switch]$Test,
    [switch]$Run,
    [switch]$QaScreenshot,
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ToolsRoot = Join-Path $ProjectRoot '.tools'
$BuildRoot = Join-Path $ProjectRoot 'build'
$CMakeVersion = '4.4.2'
$CMakeFolder = Join-Path $ToolsRoot "cmake-$CMakeVersion-windows-x86_64"
$PortableCMake = Join-Path $CMakeFolder 'bin\cmake.exe'
$ExpectedHash = 'E8139D85B3813BC38833142AE1940472E9A587E9B5D2718AC1804C60F4E57A64'

$CMakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
if ($CMakeCommand) {
    $CMake = $CMakeCommand.Source
} elseif (Test-Path -LiteralPath $PortableCMake) {
    $CMake = $PortableCMake
} else {
    New-Item -ItemType Directory -Force -Path $ToolsRoot | Out-Null
    $Archive = Join-Path $ToolsRoot "cmake-$CMakeVersion.zip"
    $Url = "https://github.com/Kitware/CMake/releases/download/v$CMakeVersion/cmake-$CMakeVersion-windows-x86_64.zip"
    Write-Host "Downloading verified portable CMake $CMakeVersion..."
    Invoke-WebRequest -Uri $Url -OutFile $Archive
    $ActualHash = (Get-FileHash -LiteralPath $Archive -Algorithm SHA256).Hash
    if ($ActualHash -ne $ExpectedHash) {
        throw "CMake archive checksum mismatch. Expected $ExpectedHash, got $ActualHash"
    }
    Expand-Archive -LiteralPath $Archive -DestinationPath $ToolsRoot -Force
    if (-not (Test-Path -LiteralPath $PortableCMake)) {
        throw "Portable CMake extraction did not produce $PortableCMake"
    }
    $CMake = $PortableCMake
}

$Compiler = Get-Command g++.exe -ErrorAction SilentlyContinue
if (-not $Compiler -and (Test-Path -LiteralPath 'E:\mingw64\bin\g++.exe')) {
    $Compiler = Get-Item -LiteralPath 'E:\mingw64\bin\g++.exe'
}
if (-not $Compiler) {
    throw 'MinGW g++.exe was not found. Add MinGW bin to PATH or install GCC 13+.'
}
$CompilerPath = $Compiler.Source
if (-not $CompilerPath) { $CompilerPath = $Compiler.FullName }
$CompilerDirectory = Split-Path -Parent $CompilerPath
$CCompiler = Join-Path $CompilerDirectory 'gcc.exe'
$MakeProgram = Join-Path $CompilerDirectory 'mingw32-make.exe'
if (-not (Test-Path -LiteralPath $CCompiler) -or -not (Test-Path -LiteralPath $MakeProgram)) {
    throw "The MinGW toolchain beside $CompilerPath is incomplete."
}

Push-Location $ProjectRoot
try {
    & $CMake -S $ProjectRoot -B $BuildRoot -G 'MinGW Makefiles' `
        "-DCMAKE_BUILD_TYPE=$Configuration" `
        "-DCMAKE_C_COMPILER=$CCompiler" `
        "-DCMAKE_CXX_COMPILER=$CompilerPath" `
        "-DCMAKE_MAKE_PROGRAM=$MakeProgram"
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    & $CMake --build $BuildRoot --parallel
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }

    $Executable = Join-Path $BuildRoot 'bin\BattlefrontBreakout.exe'
    if ($Test) {
        & (Join-Path (Split-Path -Parent $CMake) 'ctest.exe') --test-dir $BuildRoot --output-on-failure
        if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
    }
    if ($QaScreenshot) {
        $QaRoot = Join-Path $ProjectRoot 'qa'
        New-Item -ItemType Directory -Force -Path $QaRoot | Out-Null
        foreach ($Scene in @('menu', 'battle', 'bestiary', 'tutorial')) {
            $QaPath = Join-Path $QaRoot "$Scene-preview.png"
            $QaRelativePath = "..\..\qa\$Scene-preview.png"
            $RequestPath = Join-Path (Split-Path -Parent $Executable) 'qa-request.txt'
            Set-Content -LiteralPath $RequestPath -Encoding Ascii -Value @($Scene, $QaRelativePath)
            $QaProcess = Start-Process -FilePath $Executable -WorkingDirectory (Split-Path -Parent $Executable) -Wait -PassThru
            if ($QaProcess.ExitCode -ne 0) { throw "QA scene $Scene failed with exit code $($QaProcess.ExitCode)." }
            if (-not (Test-Path -LiteralPath $QaPath)) { throw "QA scene $Scene did not create a screenshot." }
        }
    }
    if ($Run) {
        Start-Process -FilePath $Executable -WorkingDirectory (Split-Path -Parent $Executable)
    }
} finally {
    Pop-Location
}
