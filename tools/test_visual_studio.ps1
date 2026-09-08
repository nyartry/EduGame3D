param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [string]$Filter,
    [switch]$List,
    [switch]$NoBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswherePath)) { throw 'Visual Studio C++ build and test tools were not found.' }
$installationPath = & $vswherePath -latest -products * -requires Microsoft.Component.MSBuild -property installationPath | Select-Object -First 1
if (!$installationPath) { throw 'Visual Studio was not found.' }
$msbuildPath = Join-Path $installationPath 'MSBuild\Current\Bin\MSBuild.exe'
$vstestPath = Join-Path $installationPath 'Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe'
if (!(Test-Path -LiteralPath $vstestPath)) { throw 'Install the Visual Studio C++ testing tools to run native tests.' }

if (!$NoBuild) {
    & $msbuildPath (Join-Path $repositoryRoot 'tests\VisualStudio\VisualStudioTests.vcxproj') /nologo /m /v:minimal "/p:Configuration=$Configuration" /p:Platform=x64
    if ($LASTEXITCODE -ne 0) { throw 'Visual Studio native test build failed.' }
}

$testDll = Join-Path $repositoryRoot "x64\$Configuration\VisualStudioTests\VisualStudioTests.dll"
if (!(Test-Path -LiteralPath $testDll)) { throw 'Build the VisualStudioTests project before running tests.' }
if ($List) {
    if ($Filter) { throw '-List cannot be combined with -Filter; VSTest does not filter discovery.' }
    & $vstestPath $testDll /Platform:x64 /ListTests
    if ($LASTEXITCODE -ne 0) { throw 'Native test discovery failed.' }
    return
}

$resultsDirectory = Join-Path $repositoryRoot "x64\$Configuration\VisualStudioTests\TestResults"
$resultName = 'VisualStudioTests-' + [guid]::NewGuid().ToString('N') + '.trx'
$testArguments = @($testDll, '/Platform:x64', "/ResultsDirectory:$resultsDirectory", "/Logger:trx;LogFileName=$resultName")
if ($Filter) { $testArguments += "/TestCaseFilter:$Filter" }
& $vstestPath @testArguments
if ($LASTEXITCODE -ne 0) { throw 'Visual Studio native tests failed.' }

# VSTest can exit successfully when no tests were discovered or matched.
$resultPath = Join-Path $resultsDirectory $resultName
if (!(Test-Path -LiteralPath $resultPath)) { throw 'VSTest did not produce a result file.' }
[xml]$result = Get-Content -LiteralPath $resultPath -Raw
if ([int]$result.TestRun.ResultSummary.Counters.total -eq 0) { throw 'No native tests were executed. Check discovery and the filter.' }
Write-Output "Native test results: $resultPath"
