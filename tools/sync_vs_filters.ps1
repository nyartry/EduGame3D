param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repository = (Resolve-Path -LiteralPath $RepositoryRoot).Path.TrimEnd('\')

function New-FilterGroup {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet('ClCompile', 'ClInclude', 'None')]
        [string]$ItemType,

        [Parameter(Mandatory = $true)]
        [string]$SearchRoot,

        [Parameter(Mandatory = $true)]
        [string]$Pattern,

        [Parameter(Mandatory = $true)]
        [string]$FilterRoot,

        [string]$FilterPrefix = '',

        [string]$ProjectBlock = ''
    )

    [pscustomobject]@{
        ItemType    = $ItemType
        SearchRoot  = $SearchRoot
        Pattern     = $Pattern
        FilterRoot  = $FilterRoot
        FilterPrefix = $FilterPrefix
        ProjectBlock = $ProjectBlock
    }
}

$projects = @(
    [pscustomobject]@{
        Output = 'EngineFramework.vcxproj.filters'
        ProjectFile = 'EngineFramework.vcxproj'
        Groups = @(
            (New-FilterGroup -ItemType ClCompile -SearchRoot 'src\Framework' -Pattern '*.cpp' -FilterRoot 'src' -ProjectBlock 'FrameworkCompile')
            (New-FilterGroup -ItemType ClInclude -SearchRoot 'src\Framework' -Pattern '*.h' -FilterRoot 'src' -ProjectBlock 'FrameworkInclude')
            (New-FilterGroup -ItemType None -SearchRoot 'src\Framework\Rendering\Shaders' -Pattern '*.hlsl' -FilterRoot 'src')
        )
    }
    [pscustomobject]@{
        Output = 'GameModule.vcxproj.filters'
        ProjectFile = 'GameModule.vcxproj'
        Groups = @(
            (New-FilterGroup -ItemType ClCompile -SearchRoot 'src\Game' -Pattern '*.cpp' -FilterRoot 'src' -ProjectBlock 'GameCompile')
            (New-FilterGroup -ItemType ClInclude -SearchRoot 'src\Game' -Pattern '*.h' -FilterRoot 'src' -ProjectBlock 'GameInclude')
        )
    }
    [pscustomobject]@{
        Output = 'GameApp.vcxproj.filters'
        ProjectFile = $null
        Groups = @(
            (New-FilterGroup -ItemType ClCompile -SearchRoot 'src\Launcher' -Pattern '*.cpp' -FilterRoot 'src')
            (New-FilterGroup -ItemType ClInclude -SearchRoot 'src\Launcher' -Pattern '*.h' -FilterRoot 'src')
            (New-FilterGroup -ItemType None -SearchRoot 'src\Framework\Rendering\Shaders' -Pattern '*.hlsl' -FilterRoot 'src')
        )
    }
    [pscustomobject]@{
        Output = 'AnimationEventEditor.vcxproj.filters'
        ProjectFile = 'AnimationEventEditor.vcxproj'
        Groups = @(
            (New-FilterGroup -ItemType ClCompile -SearchRoot 'tools\AnimationEventEditor' -Pattern '*.cpp' -FilterRoot 'tools' -ProjectBlock 'EditorCompile')
            (New-FilterGroup -ItemType ClInclude -SearchRoot 'tools\AnimationEventEditor' -Pattern '*.h' -FilterRoot 'tools' -ProjectBlock 'EditorInclude')
            (New-FilterGroup -ItemType ClCompile -SearchRoot 'third_party\imgui' -Pattern '*.cpp' -FilterRoot 'third_party' -FilterPrefix 'ThirdParty')
        )
    }
)

function Get-RepositoryRelativePath {
    param([Parameter(Mandatory = $true)][string]$FullPath)

    if (-not $FullPath.StartsWith($repository + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside the repository: $FullPath"
    }

    $FullPath.Substring($repository.Length + 1).Replace('/', '\')
}

function Get-StableFilterGuid {
    param(
        [Parameter(Mandatory = $true)][string]$Project,
        [Parameter(Mandatory = $true)][string]$Filter
    )

    $md5 = [System.Security.Cryptography.MD5]::Create()
    try {
        $inputBytes = [System.Text.Encoding]::UTF8.GetBytes("$Project|$Filter")
        $hash = $md5.ComputeHash($inputBytes)
        ([System.Guid]::new($hash)).ToString('B')
    }
    finally {
        $md5.Dispose()
    }
}

foreach ($project in $projects) {
    $items = [System.Collections.Generic.List[object]]::new()
    $filters = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $projectBlocks = @{}

    foreach ($group in $project.Groups) {
        $searchRoot = Join-Path $repository $group.SearchRoot
        $filterRoot = (Resolve-Path -LiteralPath (Join-Path $repository $group.FilterRoot)).Path.TrimEnd('\')

        foreach ($file in (Get-ChildItem -LiteralPath $searchRoot -Recurse -File -Filter $group.Pattern | Sort-Object FullName)) {
            $relativeDirectory = $file.DirectoryName.Substring($filterRoot.Length).TrimStart('\')
            $filter = if ([string]::IsNullOrWhiteSpace($group.FilterPrefix)) {
                $relativeDirectory
            }
            elseif ([string]::IsNullOrWhiteSpace($relativeDirectory)) {
                $group.FilterPrefix
            }
            else {
                "$($group.FilterPrefix)\$relativeDirectory"
            }

            if (-not [string]::IsNullOrWhiteSpace($filter)) {
                $parts = $filter.Split('\')
                for ($index = 1; $index -le $parts.Count; $index++) {
                    [void]$filters.Add(($parts[0..($index - 1)] -join '\'))
                }
            }

            $item = [pscustomobject]@{
                ItemType = $group.ItemType
                Include  = Get-RepositoryRelativePath -FullPath $file.FullName
                Filter   = $filter
            }
            $items.Add($item)

            if (-not [string]::IsNullOrWhiteSpace($group.ProjectBlock)) {
                if (-not $projectBlocks.ContainsKey($group.ProjectBlock)) {
                    $projectBlocks[$group.ProjectBlock] = [System.Collections.Generic.List[object]]::new()
                }
                $projectBlocks[$group.ProjectBlock].Add($item)
            }
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($project.ProjectFile)) {
        $projectPath = Join-Path $repository $project.ProjectFile
        $projectText = [System.IO.File]::ReadAllText($projectPath)
        $newLine = if ($projectText.Contains("`r`n")) { "`r`n" } else { "`n" }

        foreach ($blockName in ($projectBlocks.Keys | Sort-Object)) {
            $beginMarker = "<!-- BEGIN sync_vs_filters:$blockName -->"
            $endMarker = "<!-- END sync_vs_filters:$blockName -->"
            $pattern = [regex]::Escape($beginMarker) + '.*?' + [regex]::Escape($endMarker)

            if ([regex]::Matches($projectText, $pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline).Count -ne 1) {
                throw "Expected exactly one managed block '$blockName' in $($project.ProjectFile)"
            }

            $projectLines = @($projectBlocks[$blockName] | Sort-Object Include | ForEach-Object {
                "    <$($_.ItemType) Include=`"$($_.Include)`" />"
            })
            $replacement = $beginMarker + $newLine + ($projectLines -join $newLine) + $newLine + '    ' + $endMarker
            $projectText = [regex]::Replace(
                $projectText,
                $pattern,
                [System.Text.RegularExpressions.MatchEvaluator]{ param($match) $replacement },
                [System.Text.RegularExpressions.RegexOptions]::Singleline
            )
        }

        [System.IO.File]::WriteAllText($projectPath, $projectText, [System.Text.UTF8Encoding]::new($false))
    }

    $settings = [System.Xml.XmlWriterSettings]::new()
    $settings.Encoding = [System.Text.UTF8Encoding]::new($false)
    $settings.Indent = $true
    $settings.IndentChars = '  '
    $settings.NewLineChars = "`r`n"
    $settings.NewLineHandling = [System.Xml.NewLineHandling]::Replace

    $outputPath = Join-Path $repository $project.Output
    $writer = [System.Xml.XmlWriter]::Create($outputPath, $settings)
    try {
        $writer.WriteStartDocument()
        $writer.WriteStartElement('Project', 'http://schemas.microsoft.com/developer/msbuild/2003')
        $writer.WriteAttributeString('ToolsVersion', '4.0')

        $writer.WriteStartElement('ItemGroup')
        foreach ($filter in ($filters | Sort-Object)) {
            $writer.WriteStartElement('Filter')
            $writer.WriteAttributeString('Include', $filter)
            $writer.WriteElementString('UniqueIdentifier', (Get-StableFilterGuid -Project $project.Output -Filter $filter))
            $writer.WriteEndElement()
        }
        $writer.WriteEndElement()

        foreach ($itemType in @('ClCompile', 'ClInclude', 'None')) {
            $typedItems = @($items | Where-Object ItemType -eq $itemType | Sort-Object Include)
            if ($typedItems.Count -eq 0) {
                continue
            }

            $writer.WriteStartElement('ItemGroup')
            foreach ($item in $typedItems) {
                $writer.WriteStartElement($item.ItemType)
                $writer.WriteAttributeString('Include', $item.Include)
                if (-not [string]::IsNullOrWhiteSpace($item.Filter)) {
                    $writer.WriteElementString('Filter', $item.Filter)
                }
                $writer.WriteEndElement()
            }
            $writer.WriteEndElement()
        }

        $writer.WriteEndElement()
        $writer.WriteEndDocument()
    }
    finally {
        $writer.Dispose()
    }

    Write-Output "Updated $($project.Output) ($($items.Count) items, $($filters.Count) filters)"
}
