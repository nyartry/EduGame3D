param(
    [Parameter(Mandatory = $true)]
    [string]$RepositoryRoot
)

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
    @{ Pattern = 'Framework/(?:Platform/Win32|Effects/Effekseer|UI/RmlUi|Rendering/Core/Dx12)'; Message = 'Game references an engine adapter/backend.' }
)

$frameworkRules = @(
    @{ Pattern = '^\s*#include\s+[<"]Game/'; Message = 'Framework must not depend on Game.' }
)

Test-SourceRules -Directory (Join-Path $resolvedRoot 'src\Game') -Rules $gameRules
Test-SourceRules -Directory (Join-Path $resolvedRoot 'src\Framework') -Rules $frameworkRules

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
