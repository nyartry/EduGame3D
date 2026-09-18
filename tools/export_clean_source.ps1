<#
.SYNOPSIS
Exports the current EduGame3D source without Git history or the old external models.
.DESCRIPTION
Copies current tracked files and explicitly allowed new source/document/model files.
Only EduHuman, Untitled, and model notices are allowed under Content/Models.
The result contains actual LFS content, third_party dependencies and their existing
notices. It does not certify permissions for audio, effects, images or dependencies.
Existing exports require -Force and this script's matching ownership record.
.EXAMPLE
./tools/export_clean_source.ps1 -WhatIf
.EXAMPLE
./tools/export_clean_source.ps1
.EXAMPLE
./tools/export_clean_source.ps1 -OutputDirectory C:\Exports\EduGame3D -Force
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$OutputDirectory = '',
    [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot)).TrimEnd('\', '/')
$defaultExportRoot = Join-Path $repositoryRoot 'x64\CleanSource'
$exporterId = 'EduGame3D-clean-source-v1'
$utf8 = [System.Text.UTF8Encoding]::new($false)

function Read-GitOutput([string]$Arguments) {
    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo.FileName = (Get-Command git -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
    $process.StartInfo.Arguments = $Arguments
    $process.StartInfo.WorkingDirectory = $repositoryRoot
    $process.StartInfo.UseShellExecute = $false
    $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.RedirectStandardOutput = $true
    $process.StartInfo.StandardOutputEncoding = $utf8
    try {
        if (!$process.Start()) { throw 'Could not start Git.' }
        $result = $process.StandardOutput.ReadToEnd()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) { throw "Git failed to read source information: $Arguments" }
        return $result
    } finally { $process.Dispose() }
}

function Test-WithinDirectory([string]$Path, [string]$Directory) {
    return $Path.Equals($Directory, [System.StringComparison]::OrdinalIgnoreCase) -or
        $Path.StartsWith($Directory.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar,
            [System.StringComparison]::OrdinalIgnoreCase)
}

function Assert-NoReparsePoints([string]$Path) {
    $cursor = $Path
    while ($cursor) {
        $item = Get-Item -LiteralPath $cursor -Force -ErrorAction SilentlyContinue
        if ($null -ne $item -and ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint)) {
            throw "Links and junctions are not allowed in an export path: $cursor"
        }
        $parent = Split-Path -Parent $cursor
        if ($parent -eq $cursor) { break }
        $cursor = $parent
    }
}

function Assert-ExportDirectory([string]$Path) {
    if ($Path -notmatch '^[A-Za-z]:[\\/]' -or $Path -match '~[0-9]') {
        throw 'Use a full local-drive path without device, UNC, or short-name aliases for the export.'
    }
    $rawSegments = $Path.Substring([System.IO.Path]::GetPathRoot($Path).Length) -split '[\\/]'
    foreach ($segment in $rawSegments) {
        if ($segment -notin @('.', '..') -and ($segment -match '[. ]$' -or
            $segment.IndexOfAny([System.IO.Path]::GetInvalidFileNameChars()) -ge 0)) {
            throw "Ambiguous or invalid export path component: $segment"
        }
    }
    $normalized = [System.IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    $volumeRoot = [System.IO.Path]::GetPathRoot($Path).TrimEnd('\', '/')
    if ($normalized -eq $volumeRoot -or (Test-WithinDirectory $repositoryRoot $normalized)) {
        throw 'The export target cannot be a filesystem root, the source repository, or its ancestor.'
    }
    if ((Test-WithinDirectory $normalized $repositoryRoot) -and
        (!(Test-WithinDirectory $normalized $defaultExportRoot) -or $normalized -eq $defaultExportRoot)) {
        throw 'An export inside the repository must be a child of x64/CleanSource.'
    }
    $relativeToVolume = $normalized.Substring([System.IO.Path]::GetPathRoot($normalized).Length)
    foreach ($segment in ($relativeToVolume -split '[\\/]')) {
        if ($segment -match '[. ]$' -or $segment.IndexOfAny([System.IO.Path]::GetInvalidFileNameChars()) -ge 0) {
            throw "Ambiguous or invalid export path component: $segment"
        }
    }
    Assert-NoReparsePoints $normalized
}

function Test-ModelPathAllowed([string]$Path) {
    if ($Path -notmatch '^Content/Models/') { return $true }
    if ($Path -match '^Content/Models/(EduHuman|Untitled)/') {
        return $Path -match '\.(fbx|blend|gltf|glb|bin|obj|mtl|png|jpe?g|bmp|tga|dds|tiff?|webp|json|md|txt|ya?ml)$' -or
            [System.IO.Path]::GetFileName($Path) -match '^(LICENSE|COPYING|NOTICE)$'
    }
    return $Path -match '^Content/Models/(README|LICENSE|COPYING|NOTICE|CREDITS|PROVENANCE|model-provenance)([._-][^/]*)?\.(md|txt|json|ya?ml)$' -or
        $Path -match '^Content/Models/(README|LICENSE|COPYING|NOTICE|CREDITS|PROVENANCE)$'
}

function Test-ExcludedPath([string]$Path) {
    if ($Path -match '(^|/)(\.git|\.vs|\.idea|\.vscode|__pycache__|node_modules|\.venv|venv|\.cache)(/|$)') { return $true }
    if ($Path -match '\.(user|suo|pdb|ilk|idb|tlog|pyc|log|tmp|bak|blend[0-9]+)$' -or
        $Path -match '\.VC(\.VC)?\.opendb$|\.VC\.db$' -or
        [System.IO.Path]::GetFileName($Path) -in @('.env', 'imgui.ini')) { return $true }
    # Vendored Debug/Release LIBs and DLLs are dependencies, not local build output.
    if ($Path -notmatch '^third_party/' -and
        $Path -match '(^|/)(x64|x86|Debug|Release|build|obj|bin)(/|$)|\.(exe|dll|lib)$') { return $true }
    return !(Test-ModelPathAllowed $Path)
}

function Test-AllowedUntracked([string]$Path) {
    if ($Path -match '^Content/Models/') { return Test-ModelPathAllowed $Path }
    if ($Path -match '^(src|tests|tools)/') {
        return $Path -match '\.(c|cc|cpp|cxx|h|hpp|hxx|inl|hlsl|ps1|py|vcxproj|filters|props|targets|sln|md|txt|json)$'
    }
    if ($Path -match '^(docs|licenses)/') {
        return $Path -match '\.(md|txt|json|ya?ml)$' -or [System.IO.Path]::GetFileName($Path) -match '^(LICENSE|COPYING|NOTICE)$'
    }
    if ($Path -match '^third_party/') {
        return [System.IO.Path]::GetFileName($Path) -match '^(README|LICENSE|COPYING|NOTICE|CREDITS)([._-].*)?$'
    }
    return $Path -notmatch '/' -and ($Path -match '\.(sln|vcxproj|filters|props|targets)$' -or
        $Path -match '^(README|LICENSE|COPYING|NOTICE|CREDITS|THIRD_PARTY_NOTICES)([._-].*)?$')
}

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $defaultExportRoot 'EduGame3D'
} elseif (![System.IO.Path]::IsPathRooted($OutputDirectory)) {
    $OutputDirectory = Join-Path $repositoryRoot $OutputDirectory
}
Assert-ExportDirectory $OutputDirectory
$outputPath = [System.IO.Path]::GetFullPath($OutputDirectory).TrimEnd('\', '/')
$outputParent = Split-Path -Parent $outputPath
$outputName = Split-Path -Leaf $outputPath
$zipPath = Join-Path $outputParent "$outputName-clean-source.zip"
# The local ownership record is outside the distributable folder and ZIP.
$ownerPath = Join-Path $outputParent "$outputName-clean-source.owner.json"
foreach ($artifact in @($zipPath, $ownerPath)) { Assert-NoReparsePoints $artifact }

# NUL-delimited UTF-8 also preserves Japanese names under Windows PowerShell.
$tracked = @((Read-GitOutput 'ls-files --cached -z').Split([char]0) | Where-Object { $_.Length -gt 0 })
$untracked = @((Read-GitOutput 'ls-files --others --exclude-standard -z').Split([char]0) | Where-Object { $_.Length -gt 0 })
$commit = (Read-GitOutput 'rev-parse HEAD').Trim()

$fingerprintPath = Join-Path $PSScriptRoot 'model-provenance-fingerprints.json'
$fingerprints = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
$externalModels = @($fingerprints.files | Where-Object {
    $_.path -match '^Content/Models/(Player|Daven|55-rp_nathan_animated_003_walking_fbx|Y_Bot|forest_goddess)/.*\.fbx$'
})
if ($externalModels.Count -lt 8) { throw 'The denylist must contain all eight previously identified external FBX files.' }
$blockedHashes = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($entry in $externalModels) {
    if ($entry.sha256 -notmatch '^[0-9a-fA-F]{64}$') { throw "Invalid model fingerprint: $($entry.path)" }
    [void]$blockedHashes.Add($entry.sha256)
}

$candidates = @($tracked) + @($untracked | Where-Object { Test-AllowedUntracked $_ })
$selected = @($candidates | Sort-Object -Unique | Where-Object { !(Test-ExcludedPath $_) })
$files = [System.Collections.Generic.List[object]]::new()
foreach ($relativePath in $selected) {
    if ($relativePath -match '(^|/)(\.\.?)(/|$)' -or [System.IO.Path]::IsPathRooted($relativePath)) {
        throw "Invalid source path: $relativePath"
    }
    $sourcePath = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $relativePath))
    if (!(Test-WithinDirectory $sourcePath $repositoryRoot) -or (Test-WithinDirectory $sourcePath $outputPath)) {
        throw "Source path is outside the source tree or inside the export: $relativePath"
    }
    # Deletions in the working tree are intentionally absent from the export.
    if (!(Test-Path -LiteralPath $sourcePath)) { continue }
    Assert-NoReparsePoints $sourcePath
    $sourceItem = Get-Item -LiteralPath $sourcePath -Force
    if ($sourceItem.PSIsContainer) { throw "Submodules or directories cannot be copied as source files: $relativePath" }
    if ($sourceItem.Length -lt 1024 -and
        [System.IO.File]::ReadAllText($sourcePath).StartsWith('version https://git-lfs.github.com/spec/v1')) {
        throw "LFS content is not downloaded: $relativePath. Fetch its content before exporting."
    }
    $hash = (Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($blockedHashes.Contains($hash)) { throw "A known external model is present under an allowed path: $relativePath" }
    $files.Add([pscustomobject]@{ path = $relativePath.Replace('\', '/'); bytes = $sourceItem.Length; sha256 = $hash })
}
$requiredFiles = @(
    'Content/Models/EduHuman/EduHuman_Idle.fbx',
    'Content/Models/EduHuman/EduHuman_Jog.fbx',
    'Content/Models/EduHuman/EduHuman_Kick.fbx',
    'Content/Models/EduHuman/EduHuman.blend',
    'Content/Models/EduHuman/EduHuman_Kick.anim_events.json',
    'tools/generate_eduhuman.py'
)
foreach ($required in $requiredFiles) {
    if (@($files | Where-Object { $_.path -eq $required -and $_.bytes -gt 0 }).Count -ne 1) {
        throw "Required generated source/model is not ready for export: $required"
    }
}
$kickEventPath = Join-Path $repositoryRoot 'Content\Models\EduHuman\EduHuman_Kick.anim_events.json'
$kickEvents = Get-Content -LiteralPath $kickEventPath -Raw | ConvertFrom-Json
if ($kickEvents.schema -ne 'edugame3d-animation-events-v1' -or $kickEvents.sourceFbx -cne 'EduHuman_Kick.fbx') {
    throw 'EduHuman_Kick.anim_events.json must use the current schema and sibling sourceFbx EduHuman_Kick.fbx.'
}

$warnings = [System.Collections.Generic.List[string]]::new()
$warnings.Add('This export excludes the old external models. It does not certify licenses for code dependencies, audio, effects, the crest, or other assets.')
$licenseFiles = @($files | Where-Object {
    $_.path -match '^third_party/' -and [System.IO.Path]::GetFileName($_.path) -match '^(LICENSE|COPYING|NOTICE)([._-].*)?$'
} | ForEach-Object { $_.path })
foreach ($vendor in @($files | Where-Object { $_.path -match '^third_party/' } | ForEach-Object { ($_.path -split '/')[1] } | Sort-Object -Unique)) {
    if (@($licenseFiles | Where-Object { $_ -match ('^third_party/' + [regex]::Escape($vendor) + '/[^/]+$') }).Count -eq 0) {
        $warnings.Add("No standalone license/notice file is present at third_party/$vendor root; license text may exist in its headers. See docs/rights-inventory.md.")
    }
}

$existing = (Test-Path -LiteralPath $outputPath) -or (Test-Path -LiteralPath $zipPath) -or (Test-Path -LiteralPath $ownerPath)
if ($existing) {
    if (!$Force) { throw 'An export artifact already exists. Choose another output directory or use -Force for an owned export.' }
    if (!(Test-Path -LiteralPath $ownerPath -PathType Leaf)) { throw 'Refusing to replace an export without its ownership record.' }
    $owner = Get-Content -LiteralPath $ownerPath -Raw | ConvertFrom-Json
    if ($owner.exporter -ne $exporterId -or $owner.sourceRepository -ne $repositoryRoot -or
        $owner.outputDirectory -ne $outputPath -or $owner.zipPath -ne $zipPath) {
        throw 'The ownership record does not match this repository and exact output paths.'
    }
    if ((Test-Path -LiteralPath $outputPath) -and !(Test-Path -LiteralPath $outputPath -PathType Container)) {
        throw 'The export directory path is occupied by a file.'
    }
    if ((Test-Path -LiteralPath $zipPath) -and !(Test-Path -LiteralPath $zipPath -PathType Leaf)) {
        throw 'The ZIP path is occupied by a directory.'
    }
}

Write-Output "Selected $($files.Count) source files; $(@($files | Where-Object { $_.path -match '^Content/Models/' }).Count) model files/notices."
foreach ($warning in $warnings) { Write-Warning $warning }
if (!$PSCmdlet.ShouldProcess($outputPath, "Create independent source folder and $zipPath")) { return }

# Recheck every destructive target immediately before removal. Never follow a link.
Assert-ExportDirectory $outputPath
foreach ($artifact in @($zipPath, $ownerPath)) { Assert-NoReparsePoints $artifact }
if (Test-Path -LiteralPath $outputPath) {
    $pendingDirectories = [System.Collections.Generic.Stack[string]]::new()
    $pendingDirectories.Push($outputPath)
    while ($pendingDirectories.Count -gt 0) {
        foreach ($item in (Get-ChildItem -LiteralPath $pendingDirectories.Pop() -Force)) {
            if ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) { throw "Refusing to remove an export containing a link: $($item.FullName)" }
            if ($item.PSIsContainer) { $pendingDirectories.Push($item.FullName) }
        }
    }
    Remove-Item -LiteralPath $outputPath -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
[void][System.IO.Directory]::CreateDirectory($outputPath)
$owner = [ordered]@{ exporter = $exporterId; sourceRepository = $repositoryRoot; outputDirectory = $outputPath; zipPath = $zipPath; status = 'in-progress' }
[System.IO.File]::WriteAllText($ownerPath, ($owner | ConvertTo-Json), $utf8)

foreach ($file in $files) {
    $destination = [System.IO.Path]::GetFullPath((Join-Path $outputPath $file.path))
    if (!(Test-WithinDirectory $destination $outputPath)) { throw "Invalid destination: $($file.path)" }
    [void][System.IO.Directory]::CreateDirectory((Split-Path -Parent $destination))
    # Copy only the main file stream, excluding local Zone.Identifier/download metadata.
    $inputStream = [System.IO.File]::OpenRead((Join-Path $repositoryRoot $file.path))
    try {
        $outputStream = [System.IO.File]::Create($destination)
        try { $inputStream.CopyTo($outputStream) }
        finally { $outputStream.Dispose() }
    } finally { $inputStream.Dispose() }
    if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $file.sha256) {
        throw "Source changed during export: $($file.path). Retry when editing and generation are complete."
    }
}
$modelFiles = @($files | Where-Object { $_.path -match '^Content/Models/' })
$manifest = [ordered]@{
    format = 'edugame3d-model-manifest-v1'
    scope = 'SHA-256 of every included model file and notice; hashes establish identity, not permission.'
    files = $modelFiles
}
[System.IO.File]::WriteAllText((Join-Path $outputPath 'MODEL_MANIFEST.json'), ($manifest | ConvertTo-Json -Depth 6), $utf8)
$summary = [ordered]@{
    format = $exporterId
    createdUtc = [DateTime]::UtcNow.ToString('o')
    sourceCommit = $commit
    content = 'Current working-tree content, including allowed uncommitted files; no Git history or LFS object store.'
    allowedModelDirectories = @('Content/Models/EduHuman', 'Content/Models/Untitled')
    sourceFileCount = $files.Count
    includedThirdPartyNotices = $licenseFiles
    warnings = $warnings.ToArray()
}
[System.IO.File]::WriteAllText((Join-Path $outputPath 'CLEAN_SOURCE_EXPORT.json'), ($summary | ConvertTo-Json -Depth 6), $utf8)
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($outputPath, $zipPath, [System.IO.Compression.CompressionLevel]::Optimal, $true)
$owner.status = 'complete'
[System.IO.File]::WriteAllText($ownerPath, ($owner | ConvertTo-Json), $utf8)
Write-Output "Source: $outputPath"
Write-Output "Archive: $zipPath"
Write-Output "Model manifest: $(Join-Path $outputPath 'MODEL_MANIFEST.json')"
