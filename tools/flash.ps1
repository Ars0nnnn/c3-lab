#requires -Version 5.1
<#
.SYNOPSIS
    一键 build + flash + monitor（D1 产出）。

.DESCRIPTION
    在你已经 . .\tools\idf-env.ps1 之后使用；若未激活，本脚本会尝试自动激活。

.EXAMPLE
    .\tools\flash.ps1 -Port COM5

.EXAMPLE
    # 只编译不烧录（CI / 快速语法检查）
    .\tools\flash.ps1 -Port COM5 -BuildOnly

.EXAMPLE
    # 首次把某个工程切到 S3（D85）
    .\tools\flash.ps1 -Port COM5 -Project 85_s3\face -Target esp32s3

.EXAMPLE
    # 分区表改了之后：彻底清理再烧
    .\tools\flash.ps1 -Port COM5 -FullClean -Erase
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Port,

    [string]$Project = '00_bringup',

    [ValidateSet('esp32c3', 'esp32s3', 'esp32c6')]
    [string]$Target,

    [int]$Baud = 460800,

    [switch]$BuildOnly,
    [switch]$NoMonitor,
    [switch]$Erase,
    [switch]$FullClean
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

$repoRoot = Split-Path $PSScriptRoot -Parent

# ---- 1. 确保 IDF 环境可用 ---------------------------------------------------
if (-not (Get-Command 'idf.py' -ErrorAction SilentlyContinue) -and -not $env:IDF_PATH) {
    Write-Host '未检测到 ESP-IDF 环境，尝试自动激活…' -ForegroundColor Yellow
    . (Join-Path $PSScriptRoot 'idf-env.ps1') | Out-Null
}
if (-not $env:IDF_PATH) {
    throw 'IDF_PATH 未设置。请先执行：  . .\tools\idf-env.ps1'
}

$idf = Get-IdfInvocation

function Invoke-Idf {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$IdfArgs)

    $all = @()
    if ($idf.Pre) { $all += $idf.Pre }
    $all += $IdfArgs

    Write-Host ('> ' + $idf.Exe + ' ' + ($IdfArgs -join ' ')) -ForegroundColor DarkGray
    & $idf.Exe @all
    if ($LASTEXITCODE -ne 0) {
        throw ("idf.py 失败（exit={0}）：{1}" -f $LASTEXITCODE, ($IdfArgs -join ' '))
    }
}

# ---- 2. 定位工程 -------------------------------------------------------------
$projectPath = if ([System.IO.Path]::IsPathRooted($Project)) { $Project } else { Join-Path $repoRoot $Project }
if (-not (Test-Path (Join-Path $projectPath 'CMakeLists.txt'))) {
    throw ("找不到工程：{0}（缺少 CMakeLists.txt）" -f $projectPath)
}
Write-Host ("工程: {0}" -f $projectPath) -ForegroundColor Cyan

Push-Location $projectPath
try {
    # ---- 3. 可选：清理 / 切目标 ---------------------------------------------
    if ($FullClean) { Invoke-Idf fullclean }
    if ($Target) { Invoke-Idf set-target $Target }

    # ---- 4. 编译 -------------------------------------------------------------
    Invoke-Idf build

    if ($BuildOnly) {
        Write-Host '仅编译完成（-BuildOnly）。' -ForegroundColor Green
        return
    }

    # ---- 5. 烧录 -------------------------------------------------------------
    if ($Erase) { Invoke-Idf -p $Port erase-flash }
    Invoke-Idf -p $Port -b $Baud flash

    # ---- 6. 监视 -------------------------------------------------------------
    if (-not $NoMonitor) {
        Write-Host '进入 monitor（退出：Ctrl+]）' -ForegroundColor Yellow
        Invoke-Idf -p $Port monitor
    }
}
finally {
    Pop-Location
}

Write-Host '完成。' -ForegroundColor Green
