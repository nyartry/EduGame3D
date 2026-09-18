param(
    [ValidateSet('Debug', 'Release')]
    [string[]]$Configurations = @('Debug', 'Release')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$global:LASTEXITCODE = 0
$repositoryRoot = Split-Path -Parent $PSScriptRoot

& (Join-Path $PSScriptRoot 'check_architecture.ps1') -RepositoryRoot $repositoryRoot
if ($LASTEXITCODE -ne 0) { throw 'Architecture check failed.' }
& (Join-Path $PSScriptRoot 'sync_vs_filters.ps1') -RepositoryRoot $repositoryRoot -Check

$msbuildCommand = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
if ($msbuildCommand) { $msbuildPath = $msbuildCommand.Source }
else {
    $vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path -LiteralPath $vswherePath)) { throw 'Visual Studio C++ build tools were not found.' }
    $msbuildPath = & $vswherePath -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
    if (!$msbuildPath) { throw 'MSBuild was not found.' }
}

$fxc = Get-Command fxc.exe -ErrorAction SilentlyContinue
if ($fxc) { $fxcPath = $fxc.Source }
else {
    $sdkBin = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    $fxcPath = Get-ChildItem -LiteralPath $sdkBin -Directory | Where-Object Name -Match '^10\.' |
        Sort-Object { [version]$_.Name } -Descending |
        ForEach-Object { Join-Path $_.FullName 'x64\fxc.exe' } |
        Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
    if (!$fxcPath) { throw 'Windows SDK shader compiler fxc.exe was not found.' }
}

foreach ($configuration in $Configurations) {
    # The game and editor stage the same DLL into one output directory.
    # Serialize their post-build copies to avoid sharing violations.
    & $msbuildPath (Join-Path $repositoryRoot 'EduGame3D.sln') /nologo /m:1 /v:minimal "/p:Configuration=$configuration" /p:Platform=x64
    if ($LASTEXITCODE -ne 0) { throw "$configuration solution build failed." }
    foreach ($test in (Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'tests') -Filter '*Tests.vcxproj' -File | Sort-Object Name)) {
        & $msbuildPath $test.FullName /nologo /v:minimal "/p:Configuration=$configuration" /p:Platform=x64
        if ($LASTEXITCODE -ne 0) { throw "$($test.BaseName) build failed ($configuration)." }
        [string[]]$testArguments = if ($test.BaseName -in @('RenderUploadTests', 'RendererLifecycleTests', 'SkinningTests', 'AssetTests')) { @('--gpu') } else { @() }
        & (Join-Path $repositoryRoot "x64\$configuration\$($test.BaseName)\$($test.BaseName).exe") @testArguments
        if ($LASTEXITCODE -ne 0) { throw "$($test.BaseName) failed ($configuration)." }
    }
    & (Join-Path $PSScriptRoot 'test_visual_studio.ps1') -Configuration $configuration -NoBuild
    if ($LASTEXITCODE -ne 0) { throw "Native Test Explorer tests failed ($configuration)." }
    $shaderOutput = Join-Path $repositoryRoot "x64\$configuration\ShaderValidation"
    [void](New-Item -ItemType Directory -Path $shaderOutput -Force)
    foreach ($shader in (Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'src\Framework\Rendering\Shaders') -Filter '*.hlsl' -File)) {
        foreach ($stage in @(@('VSMain', 'vs_5_0'), @('PSMain', 'ps_5_0'))) {
            [string[]]$flags = if ($configuration -eq 'Debug') { @('/Zi', '/Od') } else { @('/O3') }
            & $fxcPath /nologo /WX /Ges @flags /E $stage[0] /T $stage[1] /Fo (Join-Path $shaderOutput "$($shader.BaseName).$($stage[0]).cso") $shader.FullName
            if ($LASTEXITCODE -ne 0) { throw "Shader validation failed: $($shader.Name) $($stage[0]) ($configuration)." }
        }
    }
}
Write-Output 'Core validation passed: architecture, project registration, builds, console and native regression tests and shaders.'
