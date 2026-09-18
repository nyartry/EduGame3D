[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$OutputDirectory,
    [string]$AdditionalOutputDirectory,
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repositoryPath = [System.IO.Path]::GetFullPath($RepositoryRoot)
$utf8 = [System.Text.UTF8Encoding]::new($false, $true)
$noticePaths = @(
    'third_party/assimp/LICENSE'
    'third_party/assimp/LICENSE_DEPENDENCIES.txt'
    'third_party/directxtk12/LICENSE'
    'third_party/effekseer/LICENSE.txt'
    'third_party/effekseer/LICENSE_RUNTIME_DIRECTX.txt'
    'third_party/effekseer/LICENSE_LLGI.txt'
    'third_party/effekseer/LICENSE_MSPL.txt'
    'third_party/effekseer/LICENSE_STB.txt'
    'third_party/rmlui/LICENSE.txt'
    'third_party/rmlui/include/RmlUi/Core/Containers/LICENSE.txt'
    'third_party/imgui/LICENSE.txt'
    'third_party/imgui/LICENSE_FONTS.txt'
    'third_party/imgui/LICENSE_STB.txt'
    'Content/Effects/Effekseer/Samples/LICENSE.txt'
)

# Read and validate every source before creating or changing any output.
$sourceBytes = @{}
$sourcePaths = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($relativePath in @('LICENSE') + $noticePaths) {
    $sourcePath = [System.IO.Path]::GetFullPath((Join-Path $repositoryPath $relativePath))
    if (!(Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        throw "Required distribution notice is missing: $sourcePath"
    }
    $bytes = [System.IO.File]::ReadAllBytes($sourcePath)
    if ($bytes.Length -eq 0) { throw "Distribution notice is empty: $sourcePath" }
    [void]$utf8.GetString($bytes)
    $sourceBytes[$relativePath] = $bytes
    [void]$sourcePaths.Add($sourcePath)
}

$intro = @'
EduGame3D 外部ライブラリ・素材の著作権表示と利用条件

この文書は、同梱物すべてを同じ利用条件にするものではありません。
各項目の対象には、それぞれの著作権表示・利用条件が適用されます。
EduGame3D本体の利用条件は、同じフォルダーのLICENSEを参照してください。
出所と適用範囲の補足:
https://github.com/nyartry/EduGame3D/blob/main/docs/rights-inventory.md

以下に、保存してある各許諾文・通知の全文を掲載します。
'@

$noticeStream = [System.IO.MemoryStream]::new()
try {
    $preamble = [System.Text.UTF8Encoding]::new($true).GetPreamble()
    $noticeStream.Write($preamble, 0, $preamble.Length)
    $introBytes = $utf8.GetBytes(($intro -replace "`r?`n", "`r`n") + "`r`n")
    $noticeStream.Write($introBytes, 0, $introBytes.Length)
    foreach ($relativePath in $noticePaths) {
        $heading = $utf8.GetBytes("`r`n============================================================`r`n$relativePath`r`n============================================================`r`n")
        $noticeStream.Write($heading, 0, $heading.Length)
        $original = $sourceBytes[$relativePath]
        $noticeStream.Write($original, 0, $original.Length)
        $separator = $utf8.GetBytes("`r`n")
        $noticeStream.Write($separator, 0, $separator.Length)
    }
    $noticeBytes = $noticeStream.ToArray()
} finally { $noticeStream.Dispose() }

function Write-IfChanged([string]$Path, [byte[]]$Bytes) {
    if ([System.IO.File]::Exists($Path)) {
        $existing = [System.IO.File]::ReadAllBytes($Path)
        if ($existing.Length -eq $Bytes.Length -and
            [Convert]::ToBase64String($existing) -ceq [Convert]::ToBase64String($Bytes)) {
            return
        }
    }
    $temporaryPath = $Path + '.' + [guid]::NewGuid().ToString('N') + '.tmp'
    try {
        [System.IO.File]::WriteAllBytes($temporaryPath, $Bytes)
        if ([System.IO.File]::Exists($Path)) {
            [System.IO.File]::Replace($temporaryPath, $Path, [System.Management.Automation.Language.NullString]::Value)
        } else {
            [System.IO.File]::Move($temporaryPath, $Path)
        }
    } finally {
        if ([System.IO.File]::Exists($temporaryPath)) { [System.IO.File]::Delete($temporaryPath) }
    }
}

$outputPaths = @($OutputDirectory)
if (![string]::IsNullOrWhiteSpace($AdditionalOutputDirectory)) { $outputPaths += $AdditionalOutputDirectory }
$outputPaths = @($outputPaths | ForEach-Object { [System.IO.Path]::GetFullPath($_) } | Sort-Object -Unique)
foreach ($directoryPath in $outputPaths) {
    foreach ($outputName in @('LICENSE', 'THIRD_PARTY_NOTICES.txt')) {
        $destinationPath = [System.IO.Path]::GetFullPath((Join-Path $directoryPath $outputName))
        if ($sourcePaths.Contains($destinationPath)) {
            throw "Output would overwrite an original license: $destinationPath"
        }
    }
}
foreach ($directoryPath in $outputPaths) {
    # GameApp and the editor can share an output directory during parallel builds.
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    try {
        $directoryKey = $directoryPath.TrimEnd('\', '/').ToUpperInvariant()
        $lockHash = [BitConverter]::ToString($hasher.ComputeHash($utf8.GetBytes($directoryKey))).Replace('-', '')
    } finally { $hasher.Dispose() }
    $mutex = [System.Threading.Mutex]::new($false, "Local\EduGame3D.DistributionNotices.$lockHash")
    $locked = $false
    try {
        try { $locked = $mutex.WaitOne(30000) }
        catch [System.Threading.AbandonedMutexException] { $locked = $true }
        if (!$locked) { throw "Timed out waiting to write distribution notices: $directoryPath" }
        [void][System.IO.Directory]::CreateDirectory($directoryPath)
        Write-IfChanged (Join-Path $directoryPath 'LICENSE') $sourceBytes['LICENSE']
        Write-IfChanged (Join-Path $directoryPath 'THIRD_PARTY_NOTICES.txt') $noticeBytes
    } finally {
        if ($locked) { $mutex.ReleaseMutex() }
        $mutex.Dispose()
    }
}
Write-Output "Distribution notices ready: $($outputPaths -join ', ')"
