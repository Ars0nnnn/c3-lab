#requires -Version 5.1
<#
.SYNOPSIS
    检查 C/C++ 源码是否只用"嵌入式安全"的标准库子集（docs/CXX-STYLE.md）。

.DESCRIPTION
    扫描 .c/.cpp/.cc/.h/.hpp，按三级清单判定：
      ⛔ FAIL  —— 硬禁（STL 容器 / iostream / 线程互斥 / 异常 RTTI / 裸 new-delete …）
      ⚠️ WARN  —— 谨慎（<algorithm> <memory> <optional> <expected> <chrono> <atomic> …）
    发现 ⛔ 项时退出码 1，方便挂到 CI / pre-commit。

.EXAMPLE
    .\tools\check-cpp.ps1

.EXAMPLE
    # 把"谨慎"也当错误（发布前用）
    .\tools\check-cpp.ps1 -WarningsAsErrors
#>
[CmdletBinding()]
param(
    [switch]$WarningsAsErrors,
    [string]$Root
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

if (-not $Root) { $Root = Split-Path $PSScriptRoot -Parent }

# ---- 规则表 -----------------------------------------------------------------
# 硬禁头文件：堆分配 / 宿主 OS 假设 / 体积灾难
$banIncludes = @(
    'iostream', 'sstream', 'fstream', 'iomanip', 'ios', 'streambuf',
    'string',
    'vector', 'deque', 'list', 'forward_list',
    'map', 'set', 'unordered_map', 'unordered_set', 'multimap', 'multiset',
    'functional', 'thread', 'mutex', 'condition_variable', 'future', 'semaphore', 'shared_mutex',
    'regex', 'locale', 'filesystem', 'random', 'valarray', 'complex', 'codecvt'
)

# 谨慎头文件：可用但需在 PR/笔记里交代理由
$warnIncludes = @(
    'algorithm', 'numeric', 'memory', 'optional', 'expected', 'variant', 'chrono', 'atomic', 'tuple',
    'string_view'   # 无堆，但注意它不保证 '\0' 结尾，不能直接喂给 C API
)

# 硬禁标识符（正则）。注释里的提及会被忽略（见 Strip-Comments）
$banTokens = @(
    @{ re = 'std::shared_ptr|std::weak_ptr';     why = '引用计数 + 原子操作，改用 unique_ptr 或静态生命周期' },
    @{ re = 'std::function';                     why = '可能堆分配，改用函数指针 / 模板 functor' },
    @{ re = 'std::(string|wstring)\b';           why = '堆分配，改用 char 定长缓冲 + snprintf' },
    @{ re = 'std::(vector|map|unordered_map|set|list)\b'; why = '堆容器，改用 std::array / 静态池 / constexpr 表' },
    @{ re = '\bthrow\b';                         why = '-fno-exceptions，改用 esp_err_t + [[nodiscard]]' },
    @{ re = '\bcatch\s*\(';                      why = '-fno-exceptions' },
    @{ re = '\bdynamic_cast\b|\btypeid\b';       why = '-fno-rtti，改用 enum class + switch' },
    @{ re = '(^|[^_\w])new\s+[A-Za-z_:(]';       why = '裸 new，改用静态分配或 heap_caps_malloc' },
    @{ re = '(^|[^_\w])delete\s*(\[|\w)';        why = '裸 delete' },
    @{ re = 'std::(mutex|lock_guard|unique_lock)'; why = 'FreeRTOS 有自己的原语，别混用' }
)

$skipDirs = @('build', 'managed_components', '.git', '.vscode', '.cache', 'node_modules')

# ---- 收集文件 ---------------------------------------------------------------
$files = Get-ChildItem -Path $Root -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object {
        $_.Extension -in @('.c', '.cpp', '.cc', '.h', '.hpp') -and
        -not ($_.FullName -split '[\\/]' | Where-Object { $_ -in $skipDirs })
    }

function Strip-Comments {
    param([string]$Line)
    $i = $Line.IndexOf('//')
    if ($i -ge 0) { return $Line.Substring(0, $i) }
    return $Line
}

$results = New-Object System.Collections.Generic.List[object]
function Add-Finding {
    param([string]$Level, [string]$File, [int]$Line, [string]$What, [string]$Why)
    $results.Add([pscustomobject]@{ Level = $Level; File = $File; Line = $Line; What = $What; Why = $Why })
}

foreach ($f in $files) {
    $rel = $f.FullName.Substring($Root.Length).TrimStart('\', '/')
    $lines = Get-Content -LiteralPath $f.FullName -Encoding UTF8 -ErrorAction SilentlyContinue
    if (-not $lines) { continue }

    for ($i = 0; $i -lt $lines.Count; $i++) {
        $raw = $lines[$i]
        $code = Strip-Comments $raw
        $n = $i + 1

        if ($code -match '#\s*include\s*[<"]([^">]+)[">]') {
            $hdr = $Matches[1]
            $base = [System.IO.Path]::GetFileNameWithoutExtension($hdr)
            if ($banIncludes -contains $base) {
                Add-Finding 'FAIL' $rel $n "#include <$base>" "STL 容器/流/线程 —— 见 docs/CXX-STYLE.md"
            }
            elseif ($warnIncludes -contains $base) {
                Add-Finding 'WARN' $rel $n "#include <$base>" "谨慎项：确认必要且已评估体积"
            }
            continue
        }

        foreach ($t in $banTokens) {
            if ($code -match $t.re) {
                Add-Finding 'FAIL' $rel $n ($Matches[0].Trim()) $t.why
                break
            }
        }
    }
}

# ---- 输出 -------------------------------------------------------------------
Write-Host ''
Write-Host "=== C++ 标准库子集检查（$($files.Count) 个文件）===" -ForegroundColor Cyan

$fails = $results | Where-Object { $_.Level -eq 'FAIL' }
$warns = $results | Where-Object { $_.Level -eq 'WARN' }

foreach ($r in $results) {
    $color = if ($r.Level -eq 'FAIL') { 'Red' } else { 'Yellow' }
    $mark = if ($r.Level -eq 'FAIL') { 'X' } else { '!' }
    Write-Host ("  [$mark] {0}:{1}  {2}" -f $r.File, $r.Line, $r.What) -ForegroundColor $color
    Write-Host ("       {0}" -f $r.Why) -ForegroundColor DarkGray
}

if (-not $fails -and -not $warns) {
    Write-Host "  PASS  未发现禁用项（也没有谨慎项）" -ForegroundColor Green
}

Write-Host ''
$failCount = @($fails).Count
$warnCount = @($warns).Count
Write-Host ("合计: FAIL={0}  WARN={1}" -f $failCount, $warnCount) -ForegroundColor $(if ($failCount) { 'Red' } elseif ($warnCount) { 'Yellow' } else { 'Green' })

if ($failCount -gt 0) { exit 1 }
if ($WarningsAsErrors -and $warnCount -gt 0) { exit 1 }
exit 0
