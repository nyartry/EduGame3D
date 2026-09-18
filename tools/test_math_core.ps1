param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug'
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

& $msbuildPath (Join-Path $repositoryRoot 'tests\MathCoreTests.vcxproj') /nologo /v:minimal "/p:Configuration=$Configuration" /p:Platform=x64
if ($LASTEXITCODE -ne 0) { throw "Math core test build failed (exit $LASTEXITCODE)." }

& (Join-Path $repositoryRoot "x64\$Configuration\MathCoreTests\MathCoreTests.exe")
if ($LASTEXITCODE -ne 0) { throw "Math core regression tests failed (exit $LASTEXITCODE)." }
