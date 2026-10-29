# c3-lab

ESP32-C3 从入门到精通的 112 天实验场。目标岗位：**嵌入式软件工程师（固件方向）**。

**完整逐日计划见 [`docs/PLAN.md`](docs/PLAN.md)** —— 16 周 / 112 天 / 每天 2 小时，
逐日列到"读什么、写什么、产出什么、拿什么数据验收"。

---

## 一、这个仓库是什么

不是 112 个孤立 demo，而是**一条工程血脉**：

```
P0 打通链路  →  P1 驱动层  →  P2 通信层  →  P3 系统层
                                              ↓
P7 求职产出  ←  P6 边缘计算  ←  P5 TinyML  ←  P4 底层架构
```

每个阶段产出的组件都进 `common_components/`，被后面的工程直接引用。
到 D112，你手上是**一个组件库 + 一条产品线的演进史**。

| 阶段 | 周 | 天 | 主题 | 核心产出 |
|---|---|---|---|---|
| P0 打通链路 | W1 | D1–D7 | 工具链 / 构建系统 / 烧录三件套 / 分区表 / 崩溃解析 | 可复现的工程基线 |
| P1 驱动层 | W2–W4 | D8–D28 | GPIO/UART/I2C/SPI/ADC/PWM/RMT + **手写驱动** | 5 个自写驱动组件 + `node_v0.1` |
| P2 通信层 | W5–W6 | D29–D42 | WiFi / MQTT / HTTP / BLE / NVS / 配网 / OTA | `node_v1.0`（可配网 + 可 OTA） |
| P3 系统层 | W7 | D43–D49 | FreeRTOS 调度 / 同步 / 优先级反转 / 中断延迟 / JTAG | RTOS 实测数据 |
| P4 底层架构 | W8–W9 | D50–D63 | 启动链 / 链接脚本 / 段布局 / mmap / 异常向量 / RISC-V 汇编 / Cache | `notes/arch-report.md` |
| P5 TinyML | W10–W11 | D64–D77 | 采集→训练→INT8 量化→TFLite Micro→音频 KWS→ESP-SR | 3 个模型 + 选型报告 |
| P6 边缘计算 | W12–W13 | D78–D91 | **PID 闭环** / 规则引擎 / 断网续传 + **S3 双核 SMP / 视觉 / 卸载** | 异构架构文档 |
| P7 旗舰与求职 | W14–W16 | D92–D112 | 集成 / 可靠性 / CI / 文档 / 简历 / 八股 | 可演示系统 + 求职包 |

---

## 二、环境准备（D1 前置，只需一次）

任选一种装 ESP-IDF（v5.1+）：

```powershell
# 方式 A：官方 Windows 安装器（最省事）
#   下载 https://dl.espressif.com/dl/esp-idf/ 的 esp-idf-tools-setup-online
#   安装时勾选 ESP32-C3 与 ESP32-S3 相关工具链

# 方式 B：源码安装
mkdir $HOME\esp -Force; cd $HOME\esp
git clone -b v5.3 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
.\install.ps1 esp32c3
```

装好后**不需要**每次手动 dot-source，本仓库的脚本会自动找：

```powershell
cd E:\c3-lab
. .\tools\idf-env.ps1      # 自动探测并激活 ESP-IDF（找不到会告诉你试了哪些路径）
idf.py --version           # 期望 v5.x
```

---

## 三、快速开始（三条命令）

```powershell
# 1. 激活环境
. .\tools\idf-env.ps1

# 2. 查 COM 口（插上板子）
Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match 'COM\d+' } | Select-Object Name

# 3. 编译 + 烧录 + 看串口
.\tools\flash.ps1 -Port COM5
```

预期输出（USB 口直连即可，不需要 USB-TTL）：

```
I (312) bringup: ==============================================
I (312) bringup:  c3-lab bringup  (P0 / D1-D3)
I (312) bringup: ==============================================
I (312) bringup:  chip      : ESP32-C3, 1 core(s), rev v0.4
I (312) bringup:  features  : wifi=yes ble=yes embedded-flash=yes
I (312) bringup:  idf       : v5.3.x
I (312) bringup:  heap      : free=... internal-free=... largest-block=...
I (312) bringup:  reset-rsn : 1  (1=power-on 3=sw 4=panic 5=int-wdt 7=task-wdt)
I (1312) bringup: alive 1s | heap free=... | heap min-ever=...
```

看到这个横幅 = **D3 的核心验收通过**（Console 已重定向到 USB Serial/JTAG）。

---

## 四、工具脚本

| 脚本 | 作用 | 常用写法 |
|---|---|---|
| `tools/idf-env.ps1` | 探测并激活 ESP-IDF（**必须点源**） | `. .\tools\idf-env.ps1` |
| `tools/flash.ps1` | build + flash + monitor 一条龙 | `.\tools\flash.ps1 -Port COM5` |
| `tools/new-note.ps1` | 按模板生成当天笔记 | `.\tools\new-note.ps1 -Day 8 -Title 'GPIO 与中断'` |
| `tools/check.ps1` | 离线自检仓库基线 + 分区表算术 | `.\tools\check.ps1` |

`flash.ps1` 常用参数：

```powershell
.\tools\flash.ps1 -Port COM5 -BuildOnly                       # 只编译（CI / 快速语法检查）
.\tools\flash.ps1 -Port COM5 -Project 10_drivers\node_v01      # 编别的工程
.\tools\flash.ps1 -Port COM5 -Project 85_s3\face -Target esp32s3   # 首次切 S3（D85）
.\tools\flash.ps1 -Port COM5 -FullClean -Erase                # 改过分区表之后
.\tools\flash.ps1 -Port COM5 -NoMonitor                       # 烧完不进 monitor
```

---

## 五、仓库结构

```
E:\c3-lab\
├─ docs/PLAN.md            # ★ 112 天逐日计划（唯一权威）
├─ plans/                  # 计划的工作副本（评审用，与 docs/PLAN.md 同源）
├─ 00_bringup/             # D1–D7   P0
├─ 10_drivers/             # D8–D28  P1（node_v01 在这里）
├─ 20_connect/             # D29–D42 P2（node_v1 在这里）
├─ 30_rtos/                # D43–D49 P3
├─ 40_arch/                # D50–D63 P4
├─ 50_tinyml/              # D64–D77 P5
│   ├─ gesture/ kws/ wakeword/ anomaly/
│   └─ pc_train/           # train.py / preprocess.py / models/（PC 侧，Python）
├─ 60_edge/                # D78–D84 P6（pid/ 在这里）
├─ 85_s3/                  # D85–D91 P6（异构：双核 SMP / 视觉 / 卸载）
├─ 90_flagship/            # D92–D98 P7（DESIGN.md 在这里）
├─ common_components/      # ★ 全程复用的组件库
├─ tools/                  # 本页四个脚本
└─ notes/                  # 每日 Dxxx.md + arch-report.md + tinyml-selection.md + interview.md
```

---

## 六、分区表设计（重要，D6 会重推一遍）

`00_bringup/partitions.csv` 是为 **4MB Flash** 精算的结果。两个非显而易见的决策：

**① 没有 `factory` 分区，只有 `ota_0` / `ota_1`**
4MB 装不下 `factory + ota_0 + ota_1` 三份 app。无 factory 时 bootloader 默认启动
第一个 OTA 分区，`idf.py flash` 也写 `ota_0`，行为一致；代价是无法"一键回出厂版本"。

**② `storage` 只有 512KB、`model` 只有 384KB**
双 OTA 各 1.5MB 已吃掉 3MB —— 这是 4MB 板上做 OTA + 端侧模型的真实取舍，
也正是 D59（体积优化）存在的理由。

| 分区 | 偏移 | 大小 | 用途 |
|---|---|---|---|
| nvs | 0x009000 | 16 KB | 配置 / 掉电计数 / 配网信息 |
| otadata | 0x00D000 | 8 KB | OTA 启动选择与回滚 |
| phy_init | 0x00F000 | 4 KB | RF 校准 |
| ota_0 | 0x010000 | 1.5 MB | 当前固件 |
| ota_1 | 0x190000 | 1.5 MB | OTA 目标 |
| storage | 0x310000 | 512 KB | LittleFS：日志 / 断网缓存 |
| model | 0x390000 | 384 KB | INT8 模型（mmap 直读，D71） |
| coredump | 0x3F0000 | 64 KB | 崩溃现场（D60） |

合计正好 `0x400000` = 4MB。`tools/check.ps1` 会自动验证这条算术（含重叠与空隙检测）。

> ⚠️ 改分区表后必须 `.\tools\flash.ps1 -Port COM5 -FullClean -Erase`，
> 否则旧的 `bootloader` 会按旧表启动，出现"明明改了却还是老行为"。

---

## 七、每日节奏（2 小时四段式）

| 段 | 内容 | 时长 |
|---|---|---|
| A 读 | 1 个文档章节 / 1 段官方源码，**先写 3 个疑问再看** | 30 min |
| B 写 | 手敲代码，**禁止整贴示例** | 75 min |
| C 验 | 跑通 + 测量 + 截图/日志，**没有数据不算完成** | 10 min |
| D 记 | `notes/Dxxx.md` 三行：卡点 / 根因 / 明日风险 | 5 min |

**周日固定为复盘日**，三选一：写技术博客（推荐）/ 补欠账 / 机动。
16 个周日 = 16 次缓冲，这是抗延期的唯一机制。

```powershell
.\tools\new-note.ps1 -Day 8 -Title 'GPIO 与中断'   # 生成当天笔记
```

---

## 八、Gate 一览（怎么证明没白学）

| Gate | 时机 | 通过标准 |
|---|---|---|
| P0 | D7 | 删 `build/` + `sdkconfig` 后一条命令复现 |
| P1 | D28 | `node_v01` 连跑 2h 无泄漏，可交付他人 |
| P2 | D42 | 断电 50 次 / 断网 20 次 / OTA 失败 5 次全部自恢复 |
| P3 | D49 | 10 个 RTOS 面试题能用自测数据答 |
| P4 | D63 | 10 页底层报告；能预测变量落段 3/3 |
| P5 | D77 | 3 个模型全板载可跑 + 能力边界报告 |
| P6 | D91 | C3↔S3 稳定互通 1h；双核加速比与卸载决策有量化结论 |
| 最终 | D98 | 故障注入 10/10 通过；干净 clone 一条命令构建 |
| 求职 | D105 | 简历 3 条 STAR 有量化指标；八股自测 ≥80% |

---

## 九、常见问题

| 现象 | 原因 | 处理 |
|---|---|---|
| 烧录卡在 `Connecting...._____` | 板子没进下载模式 | 按住 BOOT(GPIO9) 再按 RST；或确认 `CONFIG_ESPTOOLPY_BEFORE_RESET=y` |
| 插 USB 有电但完全没日志 | Console 还在 UART0 | 确认 `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`；改完必须 `fullclean` 重编 |
| 日志乱码 | monitor 波特率与固件不一致 | 用默认 115200，别手动改 |
| 改了 `sdkconfig.defaults` 不生效 | 已有 `sdkconfig` 优先 | 删 `sdkconfig` 或 `idf.py fullclean` |
| `idf-env.ps1` 报找不到 IDF | 装在非常规路径 | `. .\tools\idf-env.ps1 -IdfPath D:\Espressif\frameworks\esp-idf-v5.3` |
| 找不到 COM 口 | 用的是充电线 / 驱动缺失 | 换数据线；`Get-CimInstance Win32_PnPEntity` 查 |
| 编译报 Flash 装不下 | 分区表改小了 app | `idf.py size-components` 找大头，见 D59 |

---

## 十、硬件清单

| 类别 | 件 | 首次用到 |
|---|---|---|
| 主控 | ESP32-C3-DevKitM-1 / SuperMini（4MB） | D1 |
| 主控 | ESP32-S3-DevKitC-1（8MB Flash + 8MB PSRAM） | D85 |
| 传感器 | SHT30/BME280、MPU6050、INMP441、OV2640 | D12 / D64 / D72 / D87 |
| 执行器 | SSD1306 OLED、WS2812 灯环、按键×3 | D15 / D17 / D8 |
| 闭环 | PTC 加热片 20W + NTC 10K + MOS 模块 + 12V 电源 + **限温开关/保险丝** | D78 |
| 闭环延伸 | 减速电机 + AB 相编码器 + TB6612 | D81 |
| 仪器 | **USB 电流表**、**逻辑分析仪**、万用表 | D23 / D8 / D18 |

详细清单与参考价见 `docs/PLAN.md` 第三节。

---

## 十一、自检

```powershell
.\tools\check.ps1           # 离线检查：文件、配置、分区表算术、git 提交数
.\tools\check.ps1 -Strict   # 警告也算失败（可挂进 CI）
```
