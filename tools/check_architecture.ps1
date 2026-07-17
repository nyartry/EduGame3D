param(
    [Parameter(Mandatory = $true)]
    [string]$RepositoryRoot
)

$ErrorActionPreference = 'Stop'

$resolvedRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$violations = [System.Collections.Generic.List[string]]::new()

function Test-SourceRules {
    param(
        [string]$Directory,
        [array]$Rules
    )

    Get-ChildItem -LiteralPath $Directory -Recurse -File -Include *.h,*.cpp | ForEach-Object {
        $file = $_
        foreach ($rule in $Rules) {
            Select-String -LiteralPath $file.FullName -Pattern $rule.Pattern | ForEach-Object {
                $relativePath = [System.IO.Path]::GetRelativePath($resolvedRoot, $file.FullName)
                $violations.Add("${relativePath}:$($_.LineNumber): $($rule.Message)")
            }
        }
    }
}

$gameRules = @(
    @{ Pattern = '^\s*#include\s+<Windows\.h>'; Message = 'Game must not include the Win32 API.' },
    @{ Pattern = '^\s*#include\s+<d3d12\.h>'; Message = 'Game must not include Direct3D 12.' },
    @{ Pattern = '^\s*#include.*(?:RmlUi|Effekseer|assimp)'; Message = 'Game must use an engine service instead of a vendor API.' },
    @{ Pattern = '\b(?:Dx12Renderer|ID3D12\w*|IDXGI\w*|HWND|HINSTANCE|Rml::|Effekseer::)\b'; Message = 'Game references a backend-native type.' },
    @{ Pattern = 'Framework/(?:Platform/Win32|Effects/Effekseer|UI/RmlUi|Rendering/Core/(?:Dx12|RenderResourceAccess)|Rendering/Materials/Texture2D)'; Message = 'Game references an engine adapter/backend.' }
)

$frameworkRules = @(
    @{ Pattern = '^\s*#include\s+[<"]Game/'; Message = 'Framework must not depend on Game.' }
)

Test-SourceRules -Directory (Join-Path $resolvedRoot 'src\Game') -Rules $gameRules
Test-SourceRules -Directory (Join-Path $resolvedRoot 'src\Framework') -Rules $frameworkRules

# Follow the quoted Framework includes reachable from Game. This catches
# transitive leaks that a direct scan of src/Game cannot see. DirectXMath is an
# intentional public value-type dependency; platform and vendor backends are not.
$sourceRoot = Join-Path $resolvedRoot 'src'
$pendingHeaders = [System.Collections.Generic.Queue[string]]::new()
$reachableHeaders = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase)

function Add-ReachableFrameworkIncludes {
    param([string]$SourceFile)

    Select-String -LiteralPath $SourceFile -Pattern '^\s*#include\s+"(Framework/[^\"]+)"' | ForEach-Object {
        $includePath = $_.Matches[0].Groups[1].Value.Replace('\', '/')
        if ($reachableHeaders.Add($includePath)) {
            $pendingHeaders.Enqueue($includePath)
        }
    }
}

Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'Game') -Recurse -File -Include *.h,*.cpp | ForEach-Object {
    Add-ReachableFrameworkIncludes -SourceFile $_.FullName
}

while ($pendingHeaders.Count -gt 0) {
    $includePath = $pendingHeaders.Dequeue()
    $fullPath = Join-Path $sourceRoot $includePath.Replace('/', '\')
    if (Test-Path -LiteralPath $fullPath -PathType Leaf) {
        Add-ReachableFrameworkIncludes -SourceFile $fullPath
    }
}

$publicHeaderRules = @(
    @{ Pattern = '^\s*#include\s+[<"](?:Windows\.h|d3d\w*\.h|dxgi\w*\.h|wrl/|RmlUi/|Effekseer|assimp/|Audio\.h)'; Message = 'A Framework header reachable from Game exposes a platform or vendor header.' },
    @{ Pattern = '(?:\b(?:ID3D12\w*|IDXGI\w*|aiScene|aiMesh|aiMaterial)\s*[*&]|\b(?:HWND|HINSTANCE)\s+[*&]?\s*\w+\s*[,;)=]|\b(?:Microsoft::WRL|Rml::|Effekseer::|Assimp::))'; Message = 'A Framework header reachable from Game exposes a backend-native type.' }
)

foreach ($includePath in $reachableHeaders) {
    $fullPath = Join-Path $sourceRoot $includePath.Replace('/', '\')
    if (!(Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        $violations.Add("${includePath}: Game reaches a Framework header that does not exist.")
        continue
    }
    foreach ($rule in $publicHeaderRules) {
        Select-String -LiteralPath $fullPath -Pattern $rule.Pattern | ForEach-Object {
            $violations.Add("${includePath}:$($_.LineNumber): $($rule.Message)")
        }
    }
}

$audioSystem = Join-Path $resolvedRoot 'src\Framework\Audio\AudioSystem.cpp'
Select-String -LiteralPath $audioSystem -Pattern 'Content[\\/]' | ForEach-Object {
    $violations.Add("src/Framework/Audio/AudioSystem.cpp:$($_.LineNumber): Framework audio must not own game asset paths.")
}

if ($violations.Count -gt 0) {
    Write-Host 'Architecture dependency check failed:' -ForegroundColor Red
    $violations | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    exit 1
}

Write-Host 'Architecture dependency check passed.' -ForegroundColor Green
