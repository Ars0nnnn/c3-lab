#requires -Version 5.1
<#
.SYNOPSIS
    激活 ESP-IDF 环境（在本会话内设置 IDF_PATH 与工具链 PATH）。

.DESCRIPTION
    必须用**点源**方式执行，否则环境变量不会留在当前会话：
        . .\tools\idf-env.ps1

    查找顺序（第一个命中即用）：
        1. -IdfPath 参数
        2. 已存在的 $env:IDF_PATH（且目录下真的有 export.ps1）
        3. $HOME\esp\esp-idf
        4. C:\Espressif\frameworks\esp-idf-*  （官方 Windows 安装器，取版本号最大者）
        5. C:\esp\esp-idf

.EXAMPLE
    . .\tools\idf-env.ps1
    idf.py --version

.EXAMPLE
    . .\tools\idf-env.ps1 -IdfPath D:\Espressif\frameworks\esp-idf-v5.3
#>
[CmdletBinding()]
param(
    [string]$IdfPath
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

function Test-IdfRoot {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return $false }
    return (Test-Path (Join-Path $Path 'export.ps1')) -and
           (Test-Path (Join-Path $Path 'tools\idf.py'))
}

$candidates = New-Object System.Collections.Generic.List[string]
if ($IdfPath)        { $candidates.Add($IdfPath) }
if ($env:IDF_PATH)   { $candidates.Add($env:IDF_PATH) }
if ($HOME)           { $candidates.Add((Join-Path $HOME 'esp\esp-idf')) }

foreach ($base in @('C:\Espressif\frameworks', 'C:\esp')) {
    if (Test-Path $base) {
        Get-ChildItem $base -Directory -Filter 'esp-idf*' -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object { $candidates.Add($_.FullName) }
    }
}

$root = $candidates | Where-Object { Test-IdfRoot $_ } | Select-Object -First 1

if (-not $root) {
    Write-Host ''
    Write-Host '未找到 ESP-IDF。已尝试：' -ForegroundColor Red
    $candidates | Where-Object { $_ } | ForEach-Object { Write-Host "  - $_" }
    Write-Host ''
    Write-Host '请先安装 ESP-IDF（D1 前置条件），然后任选一种方式重试：' -ForegroundColor Yellow
    Write-Host '  . .\tools\idf-env.ps1 -IdfPath <你的 esp-idf 目录>'
    Write-Host '  或设置环境变量 IDF_PATH'
    return $false
}

# 已经激活过就不重复跑 export（它比较慢）
if ($env:IDF_PATH -eq $root -and (Get-Command 'idf.py' -ErrorAction SilentlyContinue)) {
    Write-Host "ESP-IDF 已激活: $root" -ForegroundColor DarkGray
    return $true
}

Write-Host "激活 ESP-IDF: $root" -ForegroundColor Cyan
. (Join-Path $root 'export.ps1')

if (-not $env:IDF_PATH) { $env:IDF_PATH = $root }

<#
.SYNOPSIS
    返回调用 idf.py 的可执行与前置参数。
.DESCRIPTION
    Windows 下 `idf.py` 能否直接调用取决于 PATHEXT 与文件关联；为了稳，
    优先走 `python <IDF_PATH>\tools\idf.py`。flash.ps1 依赖这个函数。
#>
function Get-IdfInvocation {
    $idfPy = Join-Path $env:IDF_PATH 'tools\idf.py'
    if (Test-Path $idfPy) {
        $py = Get-Command 'python' -ErrorAction SilentlyContinue
        if ($py) {
            return [pscustomobject]@{ Exe = $py.Source; Pre = @($idfPy) }
        }
    }
    if (Get-Command 'idf.py' -ErrorAction SilentlyContinue) {
        return [pscustomobject]@{ Exe = 'idf.py'; Pre = @() }
    }
    throw 'idf.py 不可用：IDF_PATH 未正确设置，或 python 不在 PATH 中。'
}

return $true
