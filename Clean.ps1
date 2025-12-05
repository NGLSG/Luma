<#
.SYNOPSIS
    将 Android 项目的核心内容导出到新目录，排除构建产物、测试代码和本地配置。
#>

param(
    [Parameter(Mandatory = $true, Position = 0, HelpMessage = "Android 项目根目录")]
    [string]$SourcePath,

    [Parameter(Mandatory = $true, Position = 1, HelpMessage = "导出的目标目录")]
    [string]$TargetPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# 定义需要排除的目录名称（匹配任意层级）
# 新增: release, test, androidTest
$excludeDirs = @(
    '.git',
    '.gradle',
    '.idea',
    'build',
    '.cxx',
    '.externalNativeBuild',
    'captures',
    'kotlin',
    'release',      # 排除 release 目录
    'test',         # 排除单元测试目录 (src/test)
    'androidTest'   # 排除仪器测试目录 (src/androidTest)
)

# 定义需要排除的文件名称（精确匹配或通配符）
$excludeFiles = @(
    'local.properties',
    '.DS_Store',
    'Thumbs.db',
    '*.iml',
    '*.hprof'
)

function Copy-FilteredItem {
    param(
        [string]$CurrentSource,
        [string]$CurrentTarget
    )

    # 确保目标目录存在
    if (-not (Test-Path -Path $CurrentTarget)) {
        [void](New-Item -ItemType Directory -Path $CurrentTarget)
    }

    # 获取当前目录下的所有项
    $items = Get-ChildItem -Path $CurrentSource -Force

    foreach ($item in $items) {
        $targetItemPath = Join-Path -Path $CurrentTarget -ChildPath $item.Name

        if ($item.PSIsContainer) {
            # 如果是目录，检查是否在排除列表中
            if ($excludeDirs -contains $item.Name) {
                Write-Host "排除目录: $($item.FullName)" -ForegroundColor Gray
                continue
            }
            
            # 递归处理子目录
            Copy-FilteredItem -CurrentSource $item.FullName -CurrentTarget $targetItemPath
        }
        else {
            # 如果是文件，检查是否在排除列表中
            $shouldExclude = $false
            foreach ($pattern in $excludeFiles) {
                if ($item.Name -like $pattern) {
                    $shouldExclude = $true
                    break
                }
            }

            if ($shouldExclude) {
                Write-Host "排除文件: $($item.FullName)" -ForegroundColor Gray
                continue
            }

            # 复制文件
            Copy-Item -Path $item.FullName -Destination $targetItemPath -Force
        }
    }
}

# --- 主程序开始 ---

$absSource = Convert-Path -Path $SourcePath
if (-not (Test-Path -Path $absSource -PathType Container)) {
    Throw "源目录不存在: $absSource"
}

Write-Host "=== 开始导出 Android 核心内容 (无测试/Release) ===" -ForegroundColor Cyan
Write-Host "源目录: $absSource"
Write-Host "目标目录: $TargetPath"
Write-Host "已排除目录: $($excludeDirs -join ', ')" -ForegroundColor Gray
Write-Host "-----------------------------------"

try {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    Copy-FilteredItem -CurrentSource $absSource -CurrentTarget $TargetPath
    $sw.Stop()

    Write-Host "-----------------------------------"
    Write-Host "导出完成！" -ForegroundColor Green
    Write-Host "耗时: $($sw.Elapsed.ToString('hh\:mm\:ss\.fff'))" -ForegroundColor Green
}
catch {
    Write-Error "导出过程中发生错误: $_"
}