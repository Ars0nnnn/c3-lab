#requires -Version 5.1
<#
.SYNOPSIS
    按模板创建当天的笔记文件（notes\Dxxx.md）。

.DESCRIPTION
    计划里每天最后 5 分钟要写 "卡点 / 根因 / 明日风险" 三行。
    这个脚本替你省掉复制模板的时间，把天号、日期、标题填好。

.EXAMPLE
    .\tools\new-note.ps1 -Day 1 -Title '仓库与环境基线'
    .\tools\new-note.ps1 -Day 8 -Title 'GPIO 与中断' -Week 2
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateRange(1, 400)]
    [int]$Day,

    [Parameter(Mandatory = $true)]
    [string]$Title,

    [int]$Week = 0,

    [string]$Phase = ''
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

$repoRoot = Split-Path $PSScriptRoot -Parent
$notesDir = Join-Path $repoRoot 'notes'
$template = Join-Path $notesDir 'TEMPLATE.md'

if (-not (Test-Path $template)) { throw "模板不存在：$template" }
if ($Week -eq 0) { $Week = [math]::Ceiling($Day / 7) }

$target = Join-Path $notesDir ('D{0:D3}.md' -f $Day)
if (Test-Path $target) {
    Write-Host "已存在，不覆盖：$target" -ForegroundColor Yellow
    return
}

# 112 天计划的阶段划分，用于自动填 Phase
if (-not $Phase) {
    $Phase = switch ($Week) {
        { $_ -le 1 }  { 'P0 打通链路' }
        { $_ -le 4 }  { 'P1 驱动层' }
        { $_ -le 6 }  { 'P2 通信层' }
        7             { 'P3 系统层' }
        { $_ -le 9 }  { 'P4 底层架构' }
        { $_ -le 11 } { 'P5 TinyML' }
        { $_ -le 13 } { 'P6 边缘计算' }
        default       { 'P7 旗舰与求职' }
    }
}

$body = Get-Content $template -Raw -Encoding UTF8
$body = $body.Replace('{{DAY}}',   ('D{0:D3}' -f $Day))
$body = $body.Replace('{{WEEK}}',  ("W$Week"))
$body = $body.Replace('{{PHASE}}', $Phase)
$body = $body.Replace('{{TITLE}}', $Title)
$body = $body.Replace('{{DATE}}',  (Get-Date -Format 'yyyy-MM-dd'))

Set-Content -Path $target -Value $body -Encoding UTF8
Write-Host "已创建：$target" -ForegroundColor Green
