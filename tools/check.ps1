#requires -Version 5.1
<#
.SYNOPSIS
    离线自检：不需要 ESP-IDF 编译、不需要硬件，验证仓库基线是否合格。

.DESCRIPTION
    覆盖 D1 的可离线验收项：
        1. 关键文件与目录是否就位
        2. sdkconfig.defaults 是否含必需键（D3 的可复现基线）
        3. partitions.csv 算术是否正确（无重叠 / 连续 / 不超 Flash / 必需分区齐全）
        4. git 仓库与提交数（D1 要求 >= 3 个语义化 commit）
        5. ESP-IDF 是否可激活（仅提示，不失败）

    需要编译/烧录才能验证的项（如 “USB 口能看到日志”）本脚本不覆盖，
    它们属于 D2/D3 的人工验收。

.EXAMPLE
    .\tools\check.ps1

.EXAMPLE
    .\tools\check.ps1 -Strict      # 警告也算失败，可用于 CI
#>
[CmdletBinding()]
param(
    [long]$FlashSize = 0x400000,
    [int]$MinCommits = 3,
    [switch]$Strict
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

$repoRoot = Split-Path $PSScriptRoot -Parent
$script:results = New-Object System.Collections.Generic.List[object]

function Add-Result {
    param(
        [ValidateSet('PASS', 'FAIL', 'WARN', 'INFO')][string]$Level,
        [string]$Item,
        [string]$Detail = ''
    )
    $script:results.Add([pscustomobject]@{ Level = $Level; Item = $Item; Detail = $Detail })
}

function ConvertFrom-HexCell {
    param([string]$Text)
    $t = $Text.Trim().Trim('"')
    if ($t -match '^0[xX]([0-9A-Fa-f]+)$') { return [Convert]::ToInt64($Matches[1], 16) }
    if ($t -match '^([0-9]+)([KkMm])$') {
        $n = [int64]$Matches[1]
        if ($Matches[2] -in @('k', 'K')) { return $n * 1024 }
        return $n * 1024 * 1024
    }
    if ($t -match '^[0-9]+$') { return [int64]$t }
    throw "无法解析的 Offset/Size 值: '$Text'"
}

# =============================================================================
# 1. 关键文件与目录
# =============================================================================
Write-Host ''
Write-Host '=== 1. 文件与目录 ===' -ForegroundColor Cyan

$requiredFiles = @(
    'README.md',
    '.gitignore',
    'sdkconfig.defaults',
    'docs\PLAN.md',
    '00_bringup\CMakeLists.txt',
    '00_bringup\partitions.csv',
    '00_bringup\main\CMakeLists.txt',
    '00_bringup\main\app_main.c',
    'tools\idf-env.ps1',
    'tools\flash.ps1',
    'tools\new-note.ps1',
    'tools\check.ps1',
    'notes\TEMPLATE.md'
)
foreach ($f in $requiredFiles) {
    if (Test-Path (Join-Path $repoRoot $f)) { Add-Result PASS "文件 $f" }
    else { Add-Result FAIL "文件 $f" '缺失' }
}

$requiredDirs = @(
    '00_bringup', '10_drivers', '20_connect', '30_rtos', '40_arch',
    '50_tinyml', '60_edge', '85_s3', '90_flagship',
    'common_components', 'tools', 'notes', 'docs'
)
foreach ($d in $requiredDirs) {
    if (Test-Path (Join-Path $repoRoot $d)) { Add-Result PASS "目录 $d" }
    else { Add-Result WARN "目录 $d" '缺失（计划结构不完整）' }
}

# =============================================================================
# 2. sdkconfig.defaults 必需键
# =============================================================================
Write-Host '=== 2. sdkconfig.defaults ===' -ForegroundColor Cyan

$cfgPath = Join-Path $repoRoot 'sdkconfig.defaults'
$requiredKeys = @(
    'CONFIG_IDF_TARGET="esp32c3"',
    'CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y',
    'CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y',
    'CONFIG_COMPILER_OPTIMIZATION_DEBUG=y',
    'CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_ENABLE=y',
    'CONFIG_LOG_MAXIMUM_LEVEL_DEBUG=y',
    'CONFIG_PARTITION_TABLE_CUSTOM=y',
    'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"',
    'CONFIG_ESP_MAIN_TASK_STACK_SIZE=4096'
)
if (Test-Path $cfgPath) {
    $cfg = Get-Content $cfgPath -Raw -Encoding UTF8
    foreach ($k in $requiredKeys) {
        if ($cfg -match [regex]::Escape($k)) { Add-Result PASS "配置 $k" }
        else { Add-Result FAIL "配置 $k" '未出现在 sdkconfig.defaults 中' }
    }
    # 常见错误：用 `# CONFIG_X is not set` 当注释（它是有效指令）
    $falseComment = ([regex]::Matches($cfg, '(?m)^#\s*CONFIG_\w+\s+is not set\s*$')).Count
    if ($falseComment -gt 0) {
        Add-Result WARN '注释里的 is-not-set' ("发现 {0} 处 `# CONFIG_x is not set`，这在 sdkconfig.defaults 里是**有效关闭指令**，不是注释" -f $falseComment)
    }
}
else {
    Add-Result FAIL 'sdkconfig.defaults' '文件不存在'
}

# =============================================================================
# 3. partitions.csv 算术
# =============================================================================
Write-Host '=== 3. 分区表算术 ===' -ForegroundColor Cyan

$csvPath = Join-Path $repoRoot '00_bringup\partitions.csv'
if (-not (Test-Path $csvPath)) {
    Add-Result FAIL 'partitions.csv' '文件不存在'
}
else {
    try {
        $entries = @()
        $lineNo = 0
        foreach ($raw in Get-Content $csvPath -Encoding UTF8) {
            $lineNo++
            $line = $raw.Trim()
            if (-not $line) { continue }
            if ($line.StartsWith('#')) { continue }

            $f = $line -split ',' | ForEach-Object { $_.Trim() }
            if ($f.Count -lt 5) {
                Add-Result WARN "partitions.csv:${lineNo}" "列数不足，已跳过: $line"
                continue
            }

            $offset = 0L; $autoOffset = $false
            $size = 0L
            try {
                $offset = ConvertFrom-HexCell $f[3]
                $size = ConvertFrom-HexCell $f[4]
            }
            catch {
                Add-Result FAIL "partitions.csv:${lineNo}" $_.Exception.Message
                continue
            }
            if ($size -le 0) {
                Add-Result FAIL "partitions.csv:${lineNo}" "分区 $($f[0]) 的 Size 为 0"
                continue
            }

            $entries += [pscustomobject]@{
                Name   = $f[0]
                Type   = $f[1]
                SubType = $f[2]
                Offset = $offset
                Size   = $size
                End    = $offset + $size
                Line   = $lineNo
            }
        }

        if ($entries.Count -eq 0) {
            Add-Result FAIL 'partitions.csv' '没有解析到任何分区'
        }
        else {
            $sorted = $entries | Sort-Object Offset

            Write-Host ''
            Write-Host ('  {0,-10} {1,-6} {2,-10} {3,-10} {4,-10}' -f 'Name', 'Type', 'SubType', 'Offset', 'Size') -ForegroundColor DarkGray
            foreach ($e in $sorted) {
                Write-Host ('  {0,-10} {1,-6} {2,-10} {3,-10} {4,-10}' -f `
                        $e.Name, $e.Type, $e.SubType, ('0x{0:X}' -f $e.Offset), ('0x{0:X}' -f $e.Size))
            }
            Write-Host ''

            # 3.1 起始位置：分区表自身占 0x8000..0x9000
            if ($sorted[0].Offset -eq 0x9000) { Add-Result PASS '分区表起点 0x9000' }
            else { Add-Result WARN '分区表起点' ('首个分区在 0x{0:X}，常规应为 0x9000（紧跟 partition-table）' -f $sorted[0].Offset) }

            # 3.2 连续且不重叠
            $prev = $null
            foreach ($e in $sorted) {
                if ($null -ne $prev) {
                    if ($e.Offset -lt $prev.End) {
                        Add-Result FAIL '分区重叠' ('{0} 与 {1} 重叠（0x{2:X} < 0x{3:X}）' -f $prev.Name, $e.Name, $e.Offset, $prev.End)
                    }
                    elseif ($e.Offset -gt $prev.End) {
                        $gap = $e.Offset - $prev.End
                        Add-Result WARN '分区空隙' ('{0} 与 {1} 之间有 0x{2:X} ({2} B) 空隙' -f $prev.Name, $e.Name, $gap)
                    }
                }
                $prev = $e
            }
            if (-not ($script:results | Where-Object { $_.Item -in @('分区重叠', '分区空隙') })) {
                Add-Result PASS '分区连续无重叠'
            }

            # 3.3 不越过 Flash 末尾
            # 注意：Measure-Object -Maximum 返回 System.Double，必须先转成 Int64，
            # 否则下面的 {0:X} 格式说明符会抛 "Format specifier was invalid"。
            $end = [long](($sorted | Measure-Object -Property End -Maximum).Maximum)
            if ($end -le $FlashSize) {
                Add-Result PASS '分区总长' ('末端 0x{0:X} <= Flash 0x{1:X}（剩余 {2} B）' -f $end, $FlashSize, ($FlashSize - $end))
            }
            else {
                Add-Result FAIL '分区总长' ('末端 0x{0:X} 超出 Flash 0x{1:X}，超出 {2} B' -f $end, $FlashSize, ($end - $FlashSize))
            }

            # 3.4 必需分区齐全
            $names = $sorted.Name
            foreach ($need in @('nvs', 'otadata', 'ota_0', 'ota_1', 'storage', 'model', 'coredump')) {
                if ($names -contains $need) { Add-Result PASS "分区 $need" }
                else { Add-Result FAIL "分区 $need" '缺失（OTA / 模型热更新 / 崩溃还原 依赖它）' }
            }
            if ($names -contains 'factory') {
                Add-Result INFO '分区 factory' '存在 factory：注意 4MB 下与双 OTA 共存会挤压 app 空间'
            }
        }
    }
    catch {
        Add-Result FAIL 'partitions.csv 解析' $_.Exception.Message
    }
}

# =============================================================================
# 4. Git 仓库与提交数
# =============================================================================
Write-Host '=== 4. Git ===' -ForegroundColor Cyan

$gitOk = $false
if (Get-Command 'git' -ErrorAction SilentlyContinue) {
    if (Test-Path (Join-Path $repoRoot '.git')) {
        $gitOk = $true
        $count = & git -C $repoRoot rev-list --count HEAD 2>$null
        if ($LASTEXITCODE -eq 0 -and $count) {
            $n = [int]$count
            if ($n -ge $MinCommits) { Add-Result PASS '提交数' ("$n >= $MinCommits") }
            else { Add-Result FAIL '提交数' ("$n < $MinCommits（D1 要求至少 $MinCommits 个语义化 commit）") }
        }
        else {
            Add-Result FAIL '提交数' '还没有任何提交'
        }

        $dirty = & git -C $repoRoot status --porcelain 2>$null
        if ($dirty) { Add-Result WARN '工作区' ("有 {0} 个未提交改动" -f ($dirty | Measure-Object).Count) }
        else { Add-Result PASS '工作区' '干净' }
    }
    else {
        Add-Result FAIL 'Git 仓库' 'c3-lab 还不是 git 仓库（git init）'
    }
}
else {
    Add-Result WARN 'Git' '未安装 git 命令'
}

# =============================================================================
# 5. ESP-IDF 可用性（仅提示）
# =============================================================================
Write-Host '=== 5. ESP-IDF ===' -ForegroundColor Cyan

if ($env:IDF_PATH) {
    Add-Result INFO 'IDF_PATH' $env:IDF_PATH
}
else {
    Add-Result WARN 'IDF_PATH' '当前会话未激活：先执行  . .\tools\idf-env.ps1'
}
$idfCmd = Get-Command 'idf.py' -ErrorAction SilentlyContinue
if ($idfCmd) { Add-Result PASS 'idf.py' $idfCmd.Source }
else { Add-Result WARN 'idf.py' '不在 PATH 中（激活环境后可用）' }

# =============================================================================
# 汇总
# =============================================================================
Write-Host ''
Write-Host '=== 结果 ===' -ForegroundColor Cyan

foreach ($r in $script:results) {
    $color = switch ($r.Level) {
        'PASS' { 'Green' }
        'FAIL' { 'Red' }
        'WARN' { 'Yellow' }
        default { 'DarkGray' }
    }
    $suffix = if ($r.Detail) { '  — ' + $r.Detail } else { '' }
    Write-Host ('  [{0}] {1}{2}' -f $r.Level, $r.Item, $suffix) -ForegroundColor $color
}

$fail = ($script:results | Where-Object Level -EQ 'FAIL').Count
$warn = ($script:results | Where-Object Level -EQ 'WARN').Count
$pass = ($script:results | Where-Object Level -EQ 'PASS').Count

Write-Host ''
Write-Host ("  PASS={0}  WARN={1}  FAIL={2}" -f $pass, $warn, $fail) -ForegroundColor Cyan
Write-Host ''

if ($fail -gt 0) { exit 1 }
if ($Strict -and $warn -gt 0) { exit 2 }
exit 0
