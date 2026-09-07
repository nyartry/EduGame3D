param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [switch]$Gpu
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$msbuildCommand = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
if ($msbuildCommand) {
    $msbuildPath = $msbuildCommand.Source
} else {
    $vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path -LiteralPath $vswherePath)) {
        throw 'MSBuild was not found. Install Visual Studio with Desktop development with C++.'
    }
    $msbuildPath = & $vswherePath -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
    if (!$msbuildPath) { throw 'No Visual Studio MSBuild installation was found.' }
}

& $msbuildPath (Join-Path $repositoryRoot 'tests\RenderUploadTests.vcxproj') /nologo /v:minimal "/p:Configuration=$Configuration" /p:Platform=x64
if ($LASTEXITCODE -ne 0) { throw "Render upload test build failed (exit $LASTEXITCODE)." }

if ($Gpu) {
    & (Join-Path $repositoryRoot "x64\$Configuration\RenderUploadTests\RenderUploadTests.exe") --gpu
} else {
    & (Join-Path $repositoryRoot "x64\$Configuration\RenderUploadTests\RenderUploadTests.exe")
}
if ($LASTEXITCODE -ne 0) { throw "Render upload regression tests failed (exit $LASTEXITCODE)." }
