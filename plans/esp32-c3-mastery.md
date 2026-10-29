# PLAN：ESP32-C3 从入门到精通 · 112 天逐日计划

> 版本 v1.0 · 待批注
> 目标岗位：**嵌入式软件工程师**（固件方向）
> 总周期：**16 周 = 112 天**，每天 **2 小时**（总工时 ≈ 224h）
> 硬件：ESP32-C3 主板 + 传感器套件 + 一块 ESP32-S3（8MB PSRAM）
> 起点：Arduino / STM32 经验，无 RTOS / 无 IDF 经验

---

## 一、Context（为什么这样设计）

### 1.1 你的真实起点与终点

| | 现在 | 112 天后 |
|---|---|---|
| IDF | 刚装好，只会 `idf.py flash` | 能从零建带自定义分区表 + OTA + 单测 + CI 的工程 |
| 驱动 | Arduino 调库 | **能读数据手册手写驱动**，逻辑分析仪验证时序 |
| RTOS | 概念听过 | 能讲清调度/同步/优先级反转，并用测量数据证明 |
| 底层 | 黑盒 | 讲清 ROM→bootloader→app_main 全链路，读链接脚本与 map |
| AI | 会用 PyTorch 跑 demo | C3 上完整 TinyML 流水线，且说清能力边界 |
| 调试 | print 大法 | JTAG 源码级调试 + coredump 现场还原 + 周期计数 profiling |
| 求职 | 无作品 | 1 个可演示多节点系统 + 16 篇技术博客 + 可量化简历项 |

### 1.2 已核实的硬件约束（决定计划边界，不是猜测）

| 约束 | 数值 | 计划中的后果 |
|---|---|---|
| CPU | 单核 RISC-V **RV32IMC** @160MHz | **C3 无 SMP**：核相关代码在这颗芯片上永不触发 → C3 的实时性只能靠优先级/中断；**双核调度放到 S3（D86）** |
| **CPU(S3)** | **双核 Xtensa LX7** + PSRAM + PIE/SIMD | 双核调度、SMP 锁、视觉、异构卸载的落点 → 见 W13 |
| SRAM | **400KB，无 PSRAM** | Tensor arena 上限 ~100–150KB；模型必须 INT8 且 ≤40KB 才安全 |
| Flash | 典型 **4MB** | 分区表是硬约束：双 OTA 分区后单 app 只有 ~1.2MB |
| SIMD | **无 PIE / 无 AI 指令** | `esp-nn` 在 C3 上无加速；推理延迟靠裁剪模型，不靠指令 |
| 无线 | WiFi4 + **BLE5 (LE only)** | 无 BT Classic；BLE Mesh / ESP-NOW 可用 |
| 串口 | 内置 USB Serial/JTAG | 省 USB-TTL，但 Console 重定向必须配置，否则"没日志" |

### 1.3 关键事实裁决（决定了 AI 阶段怎么走）

| 事实 | 来源 | 计划含义 |
|---|---|---|
| `WakeNet9s` / `WakeNet10s` **明确支持 ESP32-C3**（免 PSRAM、免 SIMD 的降本版） | ESP-SR 组件 README | 唤醒词/命令词在 C3 本机可跑 → 保留为 D74 |
| ESP-DL 文档虽列 C3，但注记"仅适用于无需 PSRAM 的应用"，2.x 实际主攻 S3/P4 | ESP-DL 快速入门 | **C3 上放弃 ESP-DL 视觉**；人脸检测挪到 S3（D88） |
| TFLite Micro + INT8 20–80KB 模型在 C3 上可实时 | 多份实测 | C3 走"传感器时序 + 音频关键字"路线 |

> **一句话结论**：C3 = 传感 + 音频 TinyML + 低功耗边缘节点；S3 = 视觉 + 唤醒词 + 网关。两块板分工写进架构文档（D90），这不是妥协，是**真实产品中的异构设计**，面试时反而是加分项。

---

## 二、Approach（总体打法）

### 2.1 一条工程血脉，不是 112 个孤立 demo

```
P0 打通链路  →  P1 驱动层  →  P2 通信层  →  P3 系统层
                                              ↓
P7 求职产出  ←  P6 边缘计算  ←  P5 TinyML  ←  P4 底层架构
```

每个阶段产出的组件（`frame_proto` / `cfg` / `sht30` / `prof` / `bus` …）**全部进 `common_components/`**，被后续工程引用。到 D112，你手上是一个**组件库 + 一条产品线的演进史**，而不是一堆散装工程。

### 2.2 每日 2 小时的固定四段式

| 段 | 内容 | 时长 | 硬规则 |
|---|---|---|---|
| A 读 | 1 个文档章节 / 1 段官方源码 | 30min | **带着问题读**，先写 3 个疑问再看 |
| B 写 | 手敲代码 | 75min | 禁止整贴示例；示例只看不复制 |
| C 验 | 跑通 + 测量 + 截图/日志 | 10min | 没有测量数据的"完成"不算完成 |
| D 记 | `notes/Dxxx.md` 三行：卡点 / 根因 / 明日风险 | 5min | 第二天开工先读昨天这三行 |

### 2.3 记号约定

- ★ 核心任务，不可跳过
- ☆ 延伸任务，时间不够可推迟到周日
- ⚠️ 高频踩坑点，当天必须记录进笔记
- 🎯 当日验收标准（**没有数据就不算过**）

### 2.4 周日固定为"复盘日"，三选一

① 写技术博客（推荐，求职素材）② 补拖欠任务 ③ 机动休息。
**16 个周日 = 16 次缓冲**，这是抗延期的唯一机制，不要拿它当学习日。

---

## 三、硬件与工具清单

### 3.1 主控

| 板子 | 规格要求 | 用途 | 参考价 |
|---|---|---|---|
| ESP32-C3-DevKitM-1 或 C3 SuperMini | 4MB Flash | 主线全部实验 | ¥15–40 |
| ESP32-S3-DevKitC-1 (WROOM-1 N8R8) | **8MB Flash + 8MB PSRAM** | D85–D91 视觉/异构 | ¥40–70 |

### 3.2 传感器与执行器（按阶段到货即可）

| 模块 | 接口 | 首次用到 | 用途 |
|---|---|---|---|
| SHT30 或 BME280 | I2C | D12 | 温湿度驱动 + 校准精度实验 |
| MPU6050 | I2C | D64 | TinyML 手势数据集 |
| 0.96" SSD1306 OLED | I2C | D15 | 自写显示驱动 |
| WS2812 灯环 (8 灯) | 单线 | D17 | RMT 时序驱动 |
| INMP441 | I2S | D72 | 音频/关键词识别 |
| 按键 ×3、LED、电位器、电阻包 | GPIO/ADC | D8 | 基础外设 |
| 5V 电源 + 470Ω/1kΩ 电阻 | | D23 | 功耗测量、上拉计算 |
| OV2640 摄像头模块 | DVP/SPI | D87 | S3 视觉 |

### 3.3 仪器（求职面试常被问"你怎么验证的"，这些是底气）

| 工具 | 必要性 | 用途 | 参考价 |
|---|---|---|---|
| USB 电流/电压表（带 mAh 累计） | **必需** | 功耗曲线、续航实测（D23/D95） |
| 24MHz 8 通道逻辑分析仪（克隆版） | **必需** | I2C/SPI/WS2812/中断延迟测量 |
| 万用表 | **必需** | 电压/通断/电流交叉验证 |
| 示波器（学校实验室借） | 可选 | PWM 波形、电源纹波、抖动 |
| JTAG | **C3/S3 内置，无需购买** | D48 源码级调试 |

### 3.4 PID 闭环控制增购件（W12 用）

| 模块 | 接口 | 天 | 用途 | 参考价 |
|---|---|---|---|---|
| 5V PTC 陶瓷加热片（20W）+ 小铝块 | — | D78 | **主选被控对象**：热惯性大，Kp/Ki/Kd 三参数都起作用，阶跃曲线漂亮 | ¥10 |
| NTC 10K 热敏电阻 + 10K 电阻 | ADC | D78 | 快速温度反馈（比 DS18B20 快一个数量级，需 Steinhart-Hart 或查表） | ¥3 |
| MOS 低边开关模块（IRLZ44N / AO3400） | GPIO（LEDC PWM） | D78 | 调节加热功率 | ¥5 |
| 12V 2A 电源适配器 | — | D78 | 加热功率源 | ¥15 |
| **限温开关（KSD9700 75℃）+ 5A 保险丝** | — | D78 | **硬件级安全，不可省**：软件限幅再加上一层物理保护 | ¥5 |
| ☆ 直流减速电机 + AB 相霍尔编码器 | GPIO 中断 | D81 | **进阶被控对象**：转速闭环，响应快，专治"定时抖动" | ¥25 |
| ☆ TB6612 / DRV8833 电机驱动 | PWM + GPIO | D81 | 调速与正反转 | ¥8 |

> 只买前 5 项即可开工（≈¥40）；电机那一套是 D81 的 ☆ 延伸，但它在面试里更值钱：**温度环考的是抗积分饱和，转速环考的是控制周期确定性**，两个都做过才完整。

---

## 四、阶段总览

| 阶段 | 周 | 天 | 主题 | 阶段交付物 |
|---|---|---|---|---|
| **P0** 打通链路 | W1 | D1–D7 | 工具链、构建系统、烧录三件套、分区表、崩溃解析 | `00_bringup/` + 可复现 `sdkconfig.defaults` |
| **P1** 驱动层 | W2–W4 | D8–D28 | GPIO/UART/I2C/SPI/ADC/PWM/RMT + **手写驱动** + 内存/低功耗/看门狗 | `common_components/`（5 个自写驱动）+ `node_v0.1` |
| **P2** 通信层 | W5–W6 | D29–D42 | WiFi/MQTT/HTTP/BLE/NVS/配网/OTA | `node_v1.0`（可配网 + 可 OTA 的联网节点） |
| **P3** 系统层 | W7 | D43–D49 | FreeRTOS 调度/同步/优先级反转/栈/中断延迟 + JTAG | RTOS 实测数据报告 |
| **P4** 底层架构 | W8–W9 | D50–D63 | 启动链、链接脚本、段布局、内存映射、异常向量、RISC-V 汇编、Cache、体积优化、coredump | `notes/arch-report.md`（10 页底层分析） |
| **P5** TinyML | W10–W11 | D64–D77 | 采集→训练→INT8 量化→TFLite Micro→音频 KWS→ESP-SR→异常检测 | `50_tinyml/`（3 个模型 + 选型报告） |
| **P6** 边缘计算 | W12–W13 | D78–D91 | **PID 闭环控制**、规则引擎、断网续传 + **S3 双核调度 / SMP / 视觉 / ESP-NOW / 异构卸载** | `60_edge/` + 异构架构文档 |
| **P7** 旗舰与求职 | W14–W16 | D92–D112 | 系统集成、可靠性加固、CI、文档、简历、八股、模拟面试 | `90_flagship/` 可演示系统 + 求职包 |

---

## 五、逐日计划（112 天）

> 每周：周一–周六 = 6 个任务日，周日 = 复盘/博客/机动。
> 所有工程路径相对 `c3-lab/`。

### W1 · D1–D7 ｜ P0 打通链路

**周目标**：建立一套"删掉 build 目录、删掉 sdkconfig，一条命令就能复现"的工程基线。

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D1** 周一 | ★ 仓库与环境基线 | IDF 快速入门、构建系统概览 | 建 `c3-lab` git 仓库、`.gitignore`、`README.md`、`sdkconfig.defaults`、`tools/flash.ps1`（一键 build+flash+monitor） | 🎯 `idf.py --version` 正常；仓库 ≥3 个语义化 commit；`flash.ps1 -Port COM5` 可用 |
| **D2** 周二 | ★ 烧录三件套 | Bootloader & 分区表概述 | `idf.py create-project`、`set-target esp32c3`、build；手工用 `esptool.py` 分别烧 bootloader / partition-table / app | 🎯 笔记画出三个 bin 的**偏移地址与作用**；关掉 `flash.ps1` 手工烧录也能点亮 |
| **D3** 周三 | ★ menuconfig 与配置固化 ⚠️ | Kconfig 机制 | 改 Console → **USB Serial/JTAG**、Flash 4MB、`-Og`、断言、日志级别；全部落到 `sdkconfig.defaults` | 🎯 删 `sdkconfig` 后重新 build，`sdkconfig` 与之前**逐行一致**；USB 口直连能看到日志 |
| **D4** 周四 | 日志与错误处理 | `esp_log.h` / `esp_err.h` | 封装 `app_log.h`（统一 TAG、模块 ID、`ESP_ERROR_CHECK` 安全替代 `CHECK_ERR`） | 🎯 5 个等级可运行时动态开关；组件 `app_log` 可被其他工程引用 |
| **D5** 周五 | ★ 崩溃现场解析 ⚠️ | Fatal Errors 文档 | 故意制造 5 类崩溃：空指针读、非法指令跳转、除零、`assert`、栈溢出 | 🎯 每类都能用 `addr2line` 定位到**你自己的源码行号**，笔记含 5 段原始日志 |
| **D6** 周六 | ★ 自定义分区表 | Partition Tables | 手写 `partitions.csv`：`nvs / factory / ota_0 / ota_1 / storage(1M) / model(1M)`；用 `esp_partition_read/write` 读写 `storage` | 🎯 `idf.py partition-table` 输出与 csv 吻合；读回写入内容字节一致 |
| **D7** 周日 | 复盘 + 博客 #1 | | 《ESP-IDF 构建系统、烧录三件套与分区表》 | 🎯 能脱稿讲清 `app.bin` 从源码到烧录的完整路径 |

---

### W2 · D8–D14 ｜ P1 驱动层（一）GPIO / PWM / UART

**周目标**：不再"调库"，而是"按手册写"。

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D8** 周一 | ★ GPIO 与中断 | GPIO 文档 + `driver/gpio.c` 摘读 | 按键中断（`gpio_install_isr_service`）+ `esp_timer` 软件消抖 + 队列上报任务 | `10_drivers/gpio_key`。🎯 逻辑分析仪**实测抖动宽度**并据此定消抖参数（不是抄 20ms） |
| **D9** 周二 | ★ LEDC PWM | LEDC 文档 | 呼吸灯（gamma 校正）+ 扫描 freq/resolution 组合 | 🎯 产出一张 `freq × 最大resolution` 表，验证 `freq ≤ 时钟/2^res` 公式；示波器测占空比误差 |
| **D10** 周三 | ★ UART 驱动与吞吐 | UART 文档（事件队列 / `uart_driver`） | DMA 收发 + 自写环形缓冲区 | `common_components/uart_pipe`。🎯 3Mbps 连续发送 1MB **零丢包**，吞吐数字入笔记 |
| **D11** 周四 | ★ 串口协议设计 ⚠️ | 自定义帧协议实践 | `0xAA55` 帧头 + 长度 + cmd + payload + **CRC16-CCITT**；状态机解析 + 超时重同步 | `common_components/frame_proto`。🎯 注入：噪声、半帧、错 CRC、粘包、超长帧 → 全部正确重同步且不越界 |
| **D12** 周五 | ★ I2C 与 SHT30 | I2C 新驱动 API（`i2c_new_master_bus`） | 自写 SHT30 驱动（含 **CRC8** 校验、单次/周期两种模式） | `common_components/sht30`。🎯 与商用温湿度计对比误差 <0.5℃ / 3%RH |
| **D13** 周六 | ★ I2C 故障与实测 | I2C 时序、仲裁、时钟拉伸 | 故障注入：拔线、改错地址、加长飞线、从机不 ACK；观察时钟拉伸 | 🎯 每种故障都有超时保护，**总线卡死 0 次**；时序截图 3 张 |
| **D14** 周日 | 复盘 + 博客 #2 | | 《I2C：从时序图到可复用驱动》 | 🎯 组件能通过 `idf.py create-project` 被新工程引用 |

---

### W3 · D15–D21 ｜ P1 驱动层（二）显示 / SPI / RMT / ADC

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D15** 周一 | ★ SSD1306 自写驱动 ⚠️ | SSD1306 数据手册（页寻址/显存） | 完整驱动：初始化序列、显存缓冲、6x8 与 8x16 字模、`oled_printf` 风格 API、局部刷新 | `common_components/ssd1306`。🎯 实时显示温度 + 自造中文点阵；刷新无闪烁 |
| **D16** 周二 | ★ SPI 与吞吐极限 | SPI Master（事务队列 / DMA / 时序参数） | SPI 环回 40MHz 传 10KB + 可配 `dummy_bits`/`cs_ena_pretrans` | 🎯 理论 vs 实测吞吐偏差分析（解释差值来自哪里） |
| **D17** 周三 | ★ RMT 与 WS2812 | RMT 编码器 API（`rmt_new_tx_channel`） | 用 RMT 编码 WS2812 的 T0H/T1H/Treset，实现色轮 + 亮度渐变 | `common_components/ws2812`。🎯 逻辑分析仪确认 T0H=0.4µs±150ns、T1H=0.8µs±150ns |
| **D18** 周四 | ★ ADC 与校准 ⚠️ | ADC oneshot / continuous(DMA) / `adc_cali` | 多点电压测量；原始值 vs 校准值 vs 万用表三方对比；**实测 ADC2 与 WiFi 冲突** | 🎯 校准后误差 <2% FS；WiFi 开启时 ADC2 行为有明确结论 |
| **D19** 周五 | ★ 三种定时器对决 | `esp_timer` / GPTimer / FreeRTOS Timer | 同一个 1kHz 周期任务用三种实现，各跑 10 万次记录抖动分布 | 🎯 抖动表（均值/P99/最坏）+ "什么场景用哪个"的结论 |
| **D20** 周六 | ★ 组件化与单元测试 ⚠️ | Component Manager、Unity、pytest-embedded | 把 D8–D17 的驱动搬进 `components/`，加 `idf_component.yml`，写 ≥8 个 Unity 单测 | 🎯 `idf.py -T` 单测全绿；`idf.py fullclean` 后仍可编 |
| **D21** 周日 | 复盘 + 博客 #3 | | 《把 demo 变成组件：ESP-IDF 组件化与单元测试》 | **周 Gate**：组件库可被外部工程一行依赖引用 |

---

### W4 · D22–D28 ｜ P1 驱动层（三）内存 / 低功耗 / 可靠性 / 综合

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D22** 周一 | ★ 内存模型 ⚠️ | Memory Types、Heap 分配 | `heap_caps_malloc` 各 CAP 组合实验、`IRAM_ATTR` 对比、`MALLOC_CAP_DMA \| INTERNAL` 失败复现 | 🎯 分配矩阵表；能解释"为什么这个 buffer 不能放外部 RAM" |
| **D23** 周二 | ★ 低功耗实测 ⚠️ | Sleep Modes 文档 | active / light sleep / deep sleep 三态；RTC 定时唤醒 + GPIO 唤醒 | 🎯 三种状态电流实测（USB 表 + 万用表交叉验证）；唤醒周期误差 <1% |
| **D24** 周三 | ★ 看门狗 | TWDT / INT_WDT / RTC_WDT | 构造 5 类 WDT 场景：任务饿死、临界区过长、ISR 卡死、`vTaskDelay` 误用、flash 操作超时 | 🎯 每类都能读懂 panic 信息并写出修复 |
| **D25** 周四 | ★★ 综合工程：采集节点 v0.1 | 复习本周全部 | SHT30 + OLED + 按键 + `frame_proto` + console 命令行（`get/set/help/reboot`） | `10_drivers/node_v01`。🎯 串口命令全可用；连续运行 2h 无内存泄漏（`heap_caps_get_free_size` 前后一致） |
| **D26** 周五 | 代码审查 | IDF 编码风格、MISRA-C 要点 | 按自查表过一遍 `node_v01`：错误码、资源释放、超时、越界、整数溢出、`volatile` | 🎯 找出并修复 ≥10 个真实问题，记录在 `notes/D026-review.md` |
| **D27** 周六 | ☆ 硬件与测量基础 | 上拉电阻计算、去耦、纹波、探头接地 | 计算 I2C 上拉阻值并实测波形；测 3.3V 纹波；对比长短地线探头差异 | 🎯 波形图 3 张 + 一页"硬件常识"笔记（面试常问） |
| **D28** 周日 | 复盘 + 博客 #4 + 机动 | | 《低功耗实测：从 20mA 到 20µA》 | **周 Gate**：`node_v0.1` 可交付给别人使用 |

---

### W5 · D29–D35 ｜ P2 通信层（一）WiFi / HTTP / MQTT / 配置

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D29** 周一 | ★ WiFi STA 状态机 ⚠️ | WiFi 驱动 + `esp_event` | 状态机化连接：IDLE→CONNECTING→CONNECTED→BACKOFF 重试（指数退避 + 抖动） | `20_connect/wifi_sta`。🎯 拔路由器电源模拟断网，30s 内自动恢复且不刷屏 |
| **D30** 周二 | `esp_netif` / SNTP / DNS | esp_netif 文档 | 打印 IP/掩码/网关/DNS；SNTP 同步 + 单调时钟封装 | `common_components/net_check`。🎯 冷启动到拿到 IP 时间 <3s |
| **D31** 周三 | ★ HTTPS 客户端 ⚠️ | `esp_http_client`、CA bundle | GET/POST、chunked 响应、证书校验（`esp_crt_bundle`）、超时与重试 | 🎯 故意用错 CA → 请求被拒（**不是静默放行**） |
| **D32** 周四 | ★ MQTT | esp-mqtt（QoS 0/1/2、遗嘱、keepalive） | PC 起 mosquitto；实现收发 + 重连 + 离线队列（QoS1） | `common_components/mqtt_client`。🎯 断网 60s 恢复后**不丢 QoS1 消息**、不重复处理 |
| **D33** 周五 | 本地 Web 服务 | `esp_http_server`、`esp_websocket_client` | 内嵌配置页（表单 + JSON API + WebSocket 实时数据） | 🎯 手机浏览器改配置 → 写 NVS → 生效，全程不断电 |
| **D34** 周六 | ★ NVS 与配置中心 ⚠️ | NVS 文档（磨损均衡/命名空间/类型） | 配置中心：默认值、范围校验、**版本迁移**、掉电安全写入 | `common_components/cfg`。🎯 反复掉电 100 次配置完好；旧版本配置能自动迁移 |
| **D35** 周日 | 复盘 + 博客 #5 | | 《ESP32 联网的可靠性设计：重连、退避、离线队列》 | 🎯 博客里有你自己测的重连时间分布 |

---

### W6 · D36–D42 ｜ P2 通信层（二）BLE / 配网 / OTA

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D36** 周一 | ★ NimBLE 基础 | NimBLE 架构、GAP/GATT | 把 `bleprph` 改造成自定义服务：1 个可写特征（命令）+ 1 个 notify 特征（数据） | `20_connect/ble_node`。🎯 nRF Connect 读写 + notify 全通 |
| **D37** 周二 | ★ BLE 参数与功耗 | 连接参数（interval/latency/MTU）、广播包长度 | 扫参数：conn interval 7.5ms→500ms，测吞吐与平均电流 | 🎯 吞吐-功耗曲线 + "用哪个 interval" 的结论 |
| **D38** 周三 | BLE + WiFi 共存 ⚠️ | Coexistence 文档 | 同时开 BLE 广播 + WiFi MQTT 大流量，测双方吞吐衰减 | 🎯 量化报告：衰减百分比 + 调优后恢复多少 |
| **D39** 周四 | ★ 配网 | `wifi_provisioning`、`esp_local_ctrl` | SoftAP + BLE 双通道配网，配网信息落 NVS，配网失败可重入 | `20_connect/prov`。🎯 出厂状态（清空 NVS）→ 手机配网 → 联网上报，全流程可复现 |
| **D40** 周五 | ★★ OTA 与回滚 ⚠️ | OTA 文档、分区/回滚机制 | `esp_https_ota` + 自建 HTTP 服务 + 版本号/CRC 校验 + `esp_ota_mark_app_valid` 回滚 | `20_connect/ota`。🎯 升级中途断电 → 重启后**回到旧版本可用**（不砖） |
| **D41** 周六 | ★★ 综合工程：联网节点 v1.0 | 复习 P1+P2 | `node_v0.1` + 配网 + MQTT + OTA + 配置中心合并 | `20_connect/node_v1`。🎯 从出厂到联网上报全流程 **5 分钟**完成，附操作手册 |
| **D42** 周日 | 复盘 + 博客 #6 | | 《BLE 与 OTA：产品化的两条命脉》 | **阶段 Gate**：`node_v1` 断电 50 次、断网 20 次、升级失败 5 次，全部自恢复 |

---

### W7 · D43–D49 ｜ P3 系统层：FreeRTOS 内核

**周目标**：把"我知道 RTOS"变成"我能用测量数据解释 RTOS 的每个行为"。

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D43** 周一 | ★ 任务与调度器 | 任务管理 + `tasks.c` 调度核心 | 优先级实验（抢占/同优先级轮转）；`vTaskDelay` vs `vTaskDelayUntil` 抖动对比 | 🎯 抖动数据表；能解释 `configTICK_RATE_HZ=100` 下 1ms 任务的命运 |
| **D44** 周二 | ★ 同步原语选型 | queue / semaphore / mutex / event group / task notification | 同一生产者-消费者问题用 **队列 / 流缓冲 / 任务通知** 三种实现 | 🎯 吞吐与延迟对比表 → 得出选型规则（面试高频） |
| **D45** 周三 | ★ 优先级反转 ⚠️ | 优先级继承、递归锁 | 构造经典三任务反转；用带继承的 mutex 修复 | 🎯 修复前后最坏延迟数字（如 210ms → 300µs），时序图 1 张 |
| **D46** 周四 | ★ 堆与栈 ⚠️ | 堆 region、栈溢出检测 | 4 个堆 region 的分配走向；用 `uxTaskGetStackHighWaterMark` 反推最小栈；触发栈溢出 | 🎯 每任务栈大小**有实测依据**；能解释 `heap_caps` 与 `malloc` 的关系 |
| **D47** 周五 | ★ 中断与临界区 | `port.c`(RISC-V)、`portENTER_CRITICAL`、`*FromISR` | GPIO 中断打点 + 逻辑分析仪测"中断→任务收到"延迟；改变临界区长度 | 🎯 中断延迟基线 + 临界区长度的影响曲线 |
| **D48** 周六 | ★★ JTAG 源码级调试 ⚠️ | OpenOCD / GDB 集成 | `idf.py openocd` + `idf.py gdb`；断点、watchpoint、看寄存器、单步；在任务切换处观察 `pxCurrentTCB`；记录 WDT 与断点冲突的解决办法 | 🎯 能像调试 PC 程序一样调试固件；note 里写清"打断点为什么会复位" |
| **D49** 周日 | 复盘 + 博客 #7 | | 《用数据说明白：ESP32-C3 上的 RTOS 调度与延迟》 | **阶段 Gate**：能用自测数据回答 10 个 RTOS 面试题 |

---

### W8 · D50–D56 ｜ P4 底层架构（一）启动链 / 链接 / 内存映射

**周目标**：把"上电到 `app_main`"这个黑盒完全打开。

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D50** 周一 | ★★ 启动链全景 ⚠️ | ROM bootloader 文档、`components/bootloader/bootloader_start.c` | 打开 bootloader 详细日志，逐行标注每个阶段做了什么；定位 eFuse/strap pin 判定分支 | 🎯 启动阶段时序图（eFuse → ROM → 2nd boot → app） |
| **D51** 周二 | ★★ app 启动细节 | `port/cpu_start.c`（`call_start_cpu0`）、`app_startup.c` | 在 `call_start_cpu0`、`esp_startup_start_app`、`main_task` 插桩打印 | 🎯 `app_main` 之前的**函数级调用链**（≥12 层） |
| **D52** 周三 | ★ 链接脚本与段布局 ⚠️ | `sections.ld.in`、`memory.ld.in`、`build/*.project.ld` | `riscv32-esp-elf-objdump -h/-t` 列出 `.text/.rodata/.data/.bss/.iram0/.dram0`；对照 `.map` | 🎯 **预测**一个新变量落在哪个段并实测验证（连续 3 次全中） |
| **D53** 周四 | ★ 内存映射与 mmap ⚠️ | TRM Memory Map 章节、`esp_partition_mmap` | 把 `model`/`storage` 分区 mmap 进地址空间并读取 | 🎯 打印映射地址窗口，验证内容；解释"为什么能像内存一样读 Flash" |
| **D54** 周五 | ★ 异常与中断向量 | TRM 中断/异常、`esp_intr_alloc` | 注册 GPIO/UART/Timer 三类中断；读 `mtvec/mcause/mepc/mstatus` | 🎯 故意触发异常后能靠 `mcause` 判断异常类型并手写解析 |
| **D55** 周六 | ★ RISC-V 汇编与周期计数 | RV32IMC ISA 摘要 | 手写一个汇编函数（`memcpy` 或 CRC16）；用 `csrr mcycle/minstret` 做周期计数工具 | `common_components/prof`。🎯 汇编版比 C 版快且有周期数据；能读反汇编看懂编译器在干什么 |
| **D56** 周日 | 复盘 + 博客 #8 | | 《ESP32-C3 从上电到 app_main：一条链走完》 | 🎯 博客含你自己画的调用链图 |

---

### W9 · D57–D63 ｜ P4 底层架构（二）Cache / 性能 / 可靠性 / 安全

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D57** 周一 | ★ Cache 与 Flash 执行 ⚠️ | TRM Cache 章节、`spi_flash` 配置 | 热点函数加/不加 `IRAM_ATTR` 的 `mcycle` 对比；查 TRM 确认 C3 cache 容量/路数 | 🎯 性能提升百分比 + 对"为什么中断处理要放 IRAM"的解释 |
| **D58** 周二 | ★ 性能测量方法论 | `esp_cpu`、周期计数器 | 完善 `prof` 组件：区间计时、P50/P99 分布、跨任务统计 | 🎯 能给任意函数出耗时分布图 |
| **D59** 周三 | 二进制体积优化 | `idf.py size` / `size-components` / `size-files` | 逐项裁剪：日志级别、`-Os`、LTO、关掉不用的组件 | 🎯 app 体积缩小 ≥15% 且功能不变（附对比表） |
| **D60** 周四 | ★ 崩溃现场还原 ⚠️ | panic handler、coredump（flash/uart） | 开启 coredump 到 flash 分区；构造崩溃后离线解析出调用栈与关键变量 | 🎯 能从一次真实崩溃还原任务名+调用栈+寄存器 |
| **D61** 周五 | 安全基础 | Secure Boot / Flash Encryption / eFuse | 读文档 + 做风险评估；若条件允许，试一次 flash encryption（注意**不可逆**） | 🎯 一页安全清单：什么场景必须开、开了什么不能回滚 |
| **D62** 周六 | ★★ 交付：底层分析报告 | — | 把 W8–W9 的测量整理成 10 页报告 | `notes/arch-report.md`。🎯 有图、有表、有数据、有结论 |
| **D63** 周日 | 复盘 + 博客 #9 | | 《为什么我的中断有 3µs 抖动：Cache、IRAM 与临界区》 | **阶段 Gate**：报告能被非嵌入式背景的人看懂一半 |

---

### W10 · D64–D70 ｜ P5 TinyML（一）数据 → 训练 → 量化 → 部署

**周目标**：把"我会 PyTorch"变成"我能让 400KB SRAM 的单片机实时跑神经网络"。

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D64** 周一 | ★★ 数据集采集 ⚠️ | TinyML 数据采集最佳实践 | C3 以 100Hz 采 MPU6050 三轴；经串口/WiFi 传 PC 存 CSV；**5 类动作 × 50 次**，含"静止/噪声/错误动作"负样本 | 🎯 数据集 v1 + `notes/D064` 写明采集协议、标签规则、已知偏差 |
| **D65** 周二 | ★ 预处理与特征 | 时序窗口/归一化/FFT 特征 | Python：滑窗切分（重叠）、归一化、时域+频域特征提取、可视化可分性 | `pc_train/preprocess.py`。🎯 特征图显示类别可分 |
| **D66** 周三 | ★ PC 训练 | Keras 小模型设计（CNN/DSCNN） | 训练 + 混淆矩阵 + 交叉验证；记录参数量 | `pc_train/train.py`。🎯 验证集准确率 ≥90%；模型参数量 <30K |
| **D67** 周四 | ★ INT8 量化 ⚠️ | Post-training quantization、representative dataset | FP32 vs INT8（full-integer）对比：精度、大小、tensor 类型 | 🎯 精度下降 <3%，体积 ≤40KB，输入输出都是 int8 |
| **D68** 周五 | ★★ 部署到 C3 ⚠️ | `esp-tflite-micro` 例程、Op resolver | 模型转 C 数组 → `MicroInterpreter` → op resolver 按需裁剪 → arena 调参 | `50_tinyml/gesture`。🎯 板上实时推理，端到端延迟 <100ms |
| **D69** 周六 | ★ 性能调优 | Arena 内存模型、`esp-nn` 开关 | 二分法定 arena 上限；裁 op；开/关 `esp-nn` 对比（C3 无 PIE，预期收益有限） | 🎯 性能基线表：延迟/arena/CPU 占用/内存峰值 |
| **D70** 周日 | 复盘 + 博客 #10 | | 《从 CSV 到 INT8：在 400KB SRAM 里塞进一个神经网络》 | 🎯 博客含完整流水线图与踩坑清单 |

---

### W11 · D71–D77 ｜ P5 TinyML（二）模型热更新 / 音频 / 官方方案 / 选型

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D71** 周一 | ★ 模型进分区 + mmap ⚠️ | D53 的 mmap 成果 + TFLite 模型加载 | 模型放 `model` 分区，`esp_partition_mmap` 直接喂给 interpreter，支持 OTA 换模型 | 🎯 **不重新烧 app** 就能换模型并生效 |
| **D72** 周二 | ★ 音频链路：I2S + INMP441 ⚠️ | I2S 驱动（std/tdm/pdm）、24bit 对齐 | 24bit 采集 → 降采样到 16kHz → 存 WAV 到 LittleFS / 经串口回传 | 🎯 PC 播放清晰无爆音；采样率误差 <1% |
| **D73** 周三 | ★★ 自训关键词识别 (KWS) | MFCC 前端、KWS 网络结构 | MFCC 特征 + 小 CNN，训练 3 类（"开灯"/"关灯"/"噪声"）；部署复跑 D68 流水线 | `50_tinyml/kws`。🎯 现场 10 次识别 ≥8 次正确 |
| **D74** 周四 | ★ ESP-SR 官方方案 ⚠️ | ESP-SR 支持矩阵（`WakeNet9s`/`WakeNet10s` 支持 C3） | 跑通唤醒词例程 → 接业务逻辑（唤醒后开灯/上报） | `50_tinyml/wakeword`。🎯 1 小时内唤醒次数/误唤醒次数都有记录 |
| **D75** 周五 | ★ 异常检测（无监督） | 自编码器 / 统计阈值法 | 振动或电流数据上做异常检测，输出异常分数 + 报警 | 🎯 注入异常能报警，误报率有数据 |
| **D76** 周六 | ★★ 三模型横向选型报告 | — | 手势 / 关键词 / 异常 三模型在 **精度-延迟-内存-功耗** 四维对比 | `notes/tinyml-selection.md`。🎯 能回答"什么任务该用哪种方案" |
| **D77** 周日 | 复盘 + 博客 #11 | | 《ESP32-C3 到底能跑多大的模型：实测边界》 | **阶段 Gate**：3 个模型全部板载可运行 + 有边界结论 |

---

### W12 · D78–D84 ｜ P6 边缘计算（一）PID 闭环控制 + 规则 / 离线

**周目标**：把一个闭环从头到尾坐实——**硬件搭好、自己实现算法、自己整定、自己发现并解决 MCU 上的真实问题**。PID 公式三行就写完了，难的是：定时抖动会让 D 项变成放大器，积分饱和会让系统开机就冲顶，传感器噪声会让你以为 Kd 没用。这周复用 D19（定时抖动）、D47（中断延迟）、D55（周期计数）的全部测量结果。

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D78** 周一 | ★★ 闭环硬件搭建与开环辨识 ⚠️ | 功率器件基础、NTC 分压与 Steinhart-Hart、安全限幅 | 搭温控回路（加热片 + MOS + NTC + 限温开关）；手动给 0/25/50/75/100% PWM，记录开环阶跃响应到温度曲线；跑开环加热极限测试 | 🔥 **安全第一**：先接限温开关和保险丝再上电。`60_edge/pid/`。🎯 拿到开环阶跃数据（升温斜率、最大温升、时间常数 τ），离线拟合出被控对象的**一阶惯性 + 滞后**参数 |
| **D79** 周二 | ★ PID 离散化实现 | 位置式/增量式 PID、抗积分饱和（clamping / back-calculation）、不完全微分、死区补偿 | 写 `pid` 组件：位置式 + 增量式两版、实心抗积分饱和、输出限幅、微分滤波；**float vs 定点 Q15 用 D55 的 `mcycle` 对比周期数** | `common_components/pid`。🎯 两版都能跑；定点版比 float 版快且有数字；组件有单测（纯算法可 PC 侧验证） |
| **D80** 周三 | ★★ 整定与闭环实测 ⚠️ | Ziegler-Nichols 临界比例度法、手动整定经验 | 先用开环数据离线仿真出初值 → 上板 Z-N / 手动整定 → 测阶跃响应：**超调、上升时间、调节时间、稳态误差、振荡次数**；☆ 用 D33 的 WebSocket 做实时曲线 + 在线改 Kp/Ki/Kd | 🔥 三组参数对比表（保守/适中/激进）+ 曲线截图。🎯 终版：超调 <10%，稳态误差 <1%，无持续振荡 |
| **D81** 周四 | ★★ PID 在 MCU 上的真实问题 ⚠️ | 控制周期抖动对控制质量的影响、微分噪声、积分饱和、执行器死区 | 刻意破坏实验：① 把控制任务改成普通 `vTaskDelay`（周期抖动）对比 `vTaskDelayUntil` ② 微分项不滤波 vs 滤波 ③ 关掉抗饱和看开机冲顶 ④ 在控制回路里插入 5ms 阻塞调用看崩溃过程；☆ 换电机转速环重复一遍（响应快，抖动敏感度放 10 倍） | `notes/pid-mcu.md`。🎯 每种劣化都有**曲线 + 数据**；能讲清"为什么 PI 已经够用还要 D" |
| **D82** 周五 | ★ 规则引擎 + 消息总线 | 发布订阅、规则 DSL / 状态机 | JSON 规则存 NVS（`if 温度>30 且 持续60s then 报警`）+ 去抖 + 事件抑制；`bus` 组件（主题订阅、异步分发、背压） | `60_edge/rules` + `common_components/bus`。🎯 改规则**不重启**即生效；新增模块**不改老代码** |
| **D83** 周六 | ★★ 断网续传 + 时间对齐 ⚠️ | LittleFS、环形日志、SNTP 与单调时钟 | 本地环形日志 + 断网缓存 + 恢复补传 + 幂等去重；时间戳体系（UTC + 单调 tick + 漂移补偿） | 🎯 断网 30min 数据**一条不丢**不重；两节点数据时间对齐误差 <50ms |
| **D84** 周日 | 复盘 + 博客 #12 | | 《PID 不难，难的是它跑在一颗 160MHz 单核上》 | **阶段 Gate**：闭环连续稳定运行 ≥2h 无振荡、无冲顶；博客含三组整定曲线 |

> **内容搬家说明**：原「端云数据分级」归入 **D90 异构卸载**（它本质就是卸载判据）；原「功耗预算模型」归入 **D95 功耗打磨**。边缘计算主题没有缩水，只是归位。

---

### W13 · D85–D91 ｜ P6 边缘计算（二）异构：双核调度 / SMP / 视觉 / 卸载

**周目标**：C3 是单核，**双核调度必须在 S3 上练**。这一周同时回答两个面试必问的问题：「SMP 下你怎么保证临界区安全」「什么时候该把任务从一端挪到另一端」。

| 天 | 主题 | 读 | 写 | 产出 / 🎯验收 |
|---|---|---|---|---|
| **D85** 周一 | ★ S3 上手与单/双核差异 | S3 vs C3（双核 LX7 / PSRAM / PIE+SIMD / 无 SMP 对比） | 同一份代码 `set-target esp32s3` 直接编过（验证组件复用质量） | 🎯 **一套组件两款芯片**都能编过、都能跑；列出"哪些代码在 C3 上是死路" |
| **D86** 周二 | ★★ 双核调度与 SMP ⚠️ | IDF SMP 配置（`CONFIG_FREERTOS_UNICORE`）、`xTaskCreatePinnedToCore`、`portMUX`/自旋锁、`esp_ipc_call(_blocking)` | ① 任务绑核 0 / 绑核 1 / 不绑，测同一负载的吞吐与延迟 ② 跨核临界区用 `portENTER_CRITICAL(&mux)` 复现数据竞争并修复 ③ 用 `esp_ipc_call` 强制另一核跑一段代码 ④ 测跨核唤醒延迟与核间 cache 一致性 | `common_components/smp_probe`。🎯 单核 vs 双核吞吐对比表（加速比数字）；竞争 bug 的复现日志 + 修复前后差异 |
| **D87** 周三 | ★ PSRAM + 摄像头流水线 ⚠️ | `MALLOC_CAP_SPIRAM`、外部 RAM 访问代价、`esp32-camera`、DVP 时序 | 大 buffer 内部 vs PSRAM 带宽/延迟对比；采集 JPEG，扫分辨率/帧率/内存占用（**摄像头帧缓存必须进 PSRAM**，正好实证 D87 的结论） | 🎯 内存分层表 + "什么放 PSRAM"规则 + 帧率数据表（如 QVGA 25fps） |
| **D88** 周四 | ★★ 视觉推理：单核做不动 → 双核分工 | ESP-DL 模型 API | 人脸检测 + 串口输出坐标；**把采集搬到一个核、推理搬到另一个核**，对比单核基线的帧率 | `85_s3/face`。🎯 实测帧率 ≥5fps；双核分工相较单核的加速比有数字 |
| **D89** 周五 | ★ 异构通信：ESP-NOW ⚠️ | ESP-NOW 文档 | C3 ↔ S3 双向传输：ACK、重传、丢包统计、RSSI 记录 | `common_components/espnow_link`。🎯 丢包率/延迟数据表 |
| **D90** 周六 | ★★ 异构计算：任务卸载设计 | 边缘卸载策略（本地 / 邻居节点 / 云端） | 三方案实测对比：同一推理任务 ① C3 本地跑 ② 卸载到 S3 ③ 上传到 PC 云端；测**端到端延迟 / 电量 / 准确率 / 断网可用性**；写架构图 + 数据契约 + 失败降级路径 | `notes/offload-design.md`。🎯 一张四维对比表 + 一份"什么情况卸载、什么情况不卸载"的判定树；接口/超时/降级全定义 |
| **D91** 周日 | 复盘 + 博客 #13 | | 《SMP 与异构卸载：双核不是把活分两半》 | **阶段 Gate**：两块板稳定互通 ≥1 小时无断链；能脱稿讲清自旋锁/绑核/卸载决策 |

---

### W14 · D92–D98 ｜ P7 旗舰项目集成

**旗舰项目**：**「低功耗多节点环境感知与声控边缘系统」**
C3 传感节点（SHT30 + MPU6050 + 麦克风 + TinyML + deep sleep + ESP-NOW）→ C3/S3 网关（聚合 + 规则 + MQTT + OTA + Web 配置）→ PC 端 mosquitto + InfluxDB + Grafana。

| 天 | 主题 | 内容 | 🎯验收 |
|---|---|---|---|
| **D92** 周一 | ★★ 系统设计定稿 | 需求、架构、分区表、任务划分、状态机、数据契约、失败矩阵、**闭环执行回路（PID）与控制周期预算** → `90_flagship/DESIGN.md` | 🎯 设计文档自评 checklist ≥20 项全过 |
| **D93** 周二 | ★★ 多任务架构实现 | 采集/推理/通信/规则/UI 五任务 + 消息流 + 优先级分配 | 🎯 各任务 CPU 占用与栈用量达标（总 CPU <70%） |
| **D94** 周三 | ★★ 可靠性加固 ⚠️ | WDT 全覆盖、异常重启计数、降级模式（断网→本地决策，模型错→规则兜底）、OTA 回滚 | 🎯 **10 种故障注入全部自恢复**（清单见下） |
| **D95** 周四 | ★ 功耗与性能打磨 | 唤醒周期、批量上报、推理频率自适应、**功耗预算模型（推理+通信+睡眠）与续航反推**（原 D83 内容）、DVFS（若支持） | 🎯 续航提升量化（如 3.5 天 → 9 天）；估算与实测偏差 <20% |
| **D96** 周五 | ★ 自动化测试 | pytest-embedded：烧录→跑用例→断言，覆盖启动/联网/推理/OTA | 🎯 **一条命令跑完全套**，报告可读 |
| **D97** 周六 | ★ CI 与交付 | GitHub Actions：编译 + 体积守门 + 单测 + Release（tag + bin） | 🎯 干净 clone 一条命令构建成功；Release 可下载 |
| **D98** 周日 | 复盘 + 博客 #14 | 《一个能交付的固件长什么样：分区、OTA、CI 与测试》 | **阶段 Gate**：外人按 README 能跑起来 |

**10 种故障注入清单（D94 用）**：① 拔电源 ② 断 WiFi ③ 断 MQTT ④ 传感器拔线 ⑤ 模型文件损坏 ⑥ Flash 写满 ⑦ 堆耗尽 ⑧ 任务卡死 ⑨ OTA 中途断电 ⑩ 电压跌落 brownout。

---

### W15 · D99–D105 ｜ P7 工程化与求职素材

| 天 | 主题 | 内容 | 🎯验收 |
|---|---|---|---|
| **D99** 周一 | 代码质量 | cppcheck / clang-tidy 配置到全仓；MISRA 关键规则自查 | 🎯 高危告警清零，附检查报告 |
| **D100** 周二 | 文档体系 | README（架构/构建/使用/排障）+ Doxygen API 文档 + 演示脚本 | 🎯 别人照文档 30min 内跑通 |
| **D101** 周三 | ★ 简历项目提炼 | 用 STAR 写 3 条：①底层架构分析 ②TinyML 落地 ③可靠性设计 | 🎯 每条都有**可量化指标**（如"中断延迟从 12µs 降至 3.2µs"） |
| **D102** 周四 | ★ 高频八股整理 | 100 题：C 语言 / RTOS / 驱动 / 总线协议 / 网络 / 底层 | `notes/interview.md`。🎯 自测答出 ≥80% |
| **D103** 周五 | ★ 手写代码训练 | 环形缓冲、位图、状态机、CRC、单链表反转、`memcpy` 优化、字符串库 | 🎯 无编译警告 + 边界用例全过 |
| **D104** 周六 | ★★ 模拟面试与演示 | 录 10min 项目演示；自问自答 20 问（为什么用 C3 不用 S3？为什么不用 FreeRTOS 的 xxx？） | 🎯 3 分钟内讲清架构与关键取舍 |
| **D105** 周日 | 复盘 + 博客 #15 | 《从学习到可入职：我给自己的验收标准》 | 🎯 博客公开发布 ≥8 篇 |

---

### W16 · D106–D112 ｜ 收尾与定向补强

| 天 | 主题 | 内容 |
|---|---|---|
| **D106** 周一 | ★ 缺口诊断 | 按第六节"精通 10 条标准"逐条打分（1–5），列缺口清单 |
| **D107** 周二 | 定向补强 A | **BLE 协议栈深挖**：抓包看 ATT/GATT 报文、配对绑定与加密流程（若 D86 双核未达标，此日优先回补双核） |
| **D108** 周三 | 定向补强 B | **RTOS 内核源码精读**：`tasks.c` 的 `vTaskSwitchContext`、`queue.c` 的阻塞链表、SMP 下 `xTaskCreatePinnedToCore` 的仿射掩码如何参与选核 |
| **D109** 周四 | 定向补强 C | **Linux 侧能力**：串口协议对接脚本、socket、systemd 服务（嵌入式岗位常要求） |
| **D110** 周五 | ★ 对外产出 | 给开源 IDF 组件提 1 个 PR，或自家组件发 v1.1（含 CHANGELOG） |
| **D111** 周六 | ★ 总复盘 | 112 天数据回顾（代码行数/测量数据/博客数/踩坑数）+ 下一个 90 天计划 |
| **D112** 周日 | 缓冲与收官 | 清理欠账、整仓 tag `v1.0`、庆祝 |

---

## 六、"精通"的 10 条验收标准

D106 用这张表自评（1–5 分，低于 4 分的进 D107–D109 补强清单）。

| # | 标准 | 对应天 | 判定方式 |
|---|---|---|---|
| 1 | 不看教程从零建一个带自定义分区表 + OTA + 单测的 IDF 工程 | D1–D42 | 现场 2h 内做完 |
| 2 | 能读芯片手册手写任意外设驱动，并用逻辑分析仪验证时序 | D8–D21 | 给一个没做过的传感器（如 BH1750）当天写出来 |
| 3 | 能诊断任意崩溃：panic / coredump / WDT / 越界 / 栈溢出 → 定位到源码行 | D5, D24, D60 | 给一段坏代码盲定位 |
| 4 | 讲清上电到 `app_main` 全链路；读链接脚本与 map；能预测变量落哪个段 | D50–D56 | 白板画图 + 现场预测验证 |
| 5 | 用测量数据（周期/延迟分布/功耗）驱动优化，而不是靠猜 | D19, D47, D57–D59 | 每个优化结论都能拿出数据 |
| 6 | C3 上完整 TinyML 流水线（采集→训练→量化→部署→调优），并说清能力边界 | D64–D77 | 现场换一个新传感器重做一遍 |
| 7 | 设计并交付多节点、断网可用、可 OTA 的边缘系统 | D78–D98 | 故障注入 10/10 通过 |
| 8 | 能在 MCU 上实现并整定一个稳定闭环，且能说清定时抖动 / 积分饱和 / 微分噪声的处理 | D78–D81 | 温控或转速闭环阶跃响应达标（超调 <10%，稳态误差 <1%） |
| 9 | 讲清 SMP 双核调度（绑核/自旋锁/核间通信/cache 一致性）与**异构任务卸载决策**（本地 vs 邻居 vs 云） | D86, D90 | 白板画双核任务图；用自测数据论证卸载与否 |
| 10 | 能写成文档讲给别人听，面试 3 分钟讲清架构与取舍 | D100–D105 | 模拟面试通过 |

---

## 七、仓库结构与复用清单

## 7.1 目录

```
c3-lab/
├─ 00_bringup/            # D1–D7
├─ 10_drivers/            # D8–D28
│   └─ node_v01/
├─ 20_connect/            # D29–D42
│   └─ node_v1/
├─ 30_rtos/               # D43–D49
├─ 40_arch/               # D50–D63
├─ 50_tinyml/             # D64–D77
│   ├─ gesture/  kws/  wakeword/  anomaly/
│   └─ pc_train/          # train.py / preprocess.py / models/
├─ 60_edge/               # D78–D84（含 pid/）
├─ 85_s3/                 # D85–D91
├─ 90_flagship/           # D92–D98
│   └─ DESIGN.md
├─ common_components/     # ★ 全程复用
│   ├─ app_log/  frame_proto/  uart_pipe/  sht30/
│   ├─ ssd1306/  ws2812/  cfg/  mqtt_client/
│   ├─ net_check/  prof/  bus/  espnow_link/
├─ tools/                 # flash.ps1 / test_e2e.py / dataset_tool.py
└─ notes/                 # 每日 Dxxx.md + arch-report.md + tinyml-selection.md + interview.md
```

## 7.2 官方资源复用（不要造轮子，但要"抄完能讲清"）

| 用途 | 路径 |
|---|---|
| 全阶段示例库 | `$IDF_PATH/examples/` （get-started / peripherals / wifi / bluetooth / protocols / system / storage / provisioning） |
| 组件注册表（一行引依赖） | `main/idf_component.yml` → `espressif/esp-tflite-micro`、`espressif/esp-sr`、`espressif/esp-dl`、`espressif/esp32-camera` |
| 构建系统 | `$IDF_PATH/tools/cmake/project.cmake`、`components/*/CMakeLists.txt` |
| 启动链源码 | `components/bootloader/bootloader_start.c`、`components/esp_system/port/cpu_start.c`、`components/esp_system/app_startup.c` |
| 链接脚本 | `components/esp_system/ld/esp32c3/sections.ld.in`、`memory.ld.in` |
| FreeRTOS 内核 | `components/freertos/FreeRTOS-Kernel/tasks.c`、`queue.c`、`portable/riscv/port.c` |
| 测试框架 | `components/unity`、`pytest-embedded`（`idf.py -T`） |
| 目标芯片手册 | ESP32-C3 技术参考手册（Memory Map / Interrupt / Cache 章节）、数据手册 |

---

## 八、Verification（怎么证明这 112 天没白学）

| 检查点 | 时机 | 通过标准 |
|---|---|---|
| **周 Gate** | 每周日 | 当日验收 🎯 全过；本周组件能被外部工程引用 |
| **阶段 Gate P0** | D7 | 删 build + sdkconfig 后一条命令复现 |
| **阶段 Gate P1** | D28 | `node_v01` 连跑 2h 无泄漏，可交付他人 |
| **阶段 Gate P2** | D42 | 断电 50 次 / 断网 20 次 / OTA 失败 5 次全部自恢复 |
| **阶段 Gate P3** | D49 | 10 个 RTOS 面试题能用自测数据答 |
| **阶段 Gate P4** | D63 | 10 页底层报告完成，能预测变量落段 3/3 |
| **阶段 Gate P5** | D77 | 3 个模型全板载可跑 + 能力边界报告 |
| **阶段 Gate P6** | D91 | C3↔S3 稳定互通 1h；双核加速比与卸载决策有量化结论 |
| **最终 Gate** | D98 | 故障注入 10/10 通过；干净 clone 一条命令构建 |
| **求职 Gate** | D105 | 简历 3 条 STAR 有量化指标；八股自测 ≥80%；3min 讲清架构 |

---

## 九、风险与应对

| 风险 | 概率 | 应对 |
|---|---|---|
| 每日 2h 不够，普遍延期 | **高** | 16 个周日全是缓冲；☆ 任务可弃；每阶段末有 Gate 但允许"降级通过"（少做 1 个延伸任务） |
| 硬件到货慢（传感器/摄像头） | 中 | 到货前跳过对应天，先做 D50+ 的纯软件底层章节（不依赖外设） |
| TinyML 效果不达标（准确率低） | 中 | 降级路径：① 改用 Edge Impulse 现成流程 ② 退化到"二分类 + 阈值"方案 ③ 把"做不出来的原因分析"本身写成博客（面试同样是加分） |
| C3 上 ESP-SR 跑不起来 | 低-中 | 已验证 `WakeNet9s/10s` 支持 C3；若仍失败，退回 D73 自训 KWS |
| 被八股分散注意力，工程烂尾 | **高** | 八股只在 D102 集中处理；其余时间**禁止刷题**，只能写代码 |
| 陷入"完美主义重构" | 中 | 组件只允许在当前阶段内重构；过 Gate 后冻结接口，只加不改 |

---

## 十、参考来源（已核实）

1. ESP-SR 组件 README（`WakeNet9s` 支持 C3/C5，`WakeNet10s` 支持无 PIE 芯片）
   https://components.espressif.com/components/espressif/esp-sr
2. WakeNet 模型与芯片支持表（WakeNet9s 支持 ESP32C3/C5/C6）
   https://docs.espressif.com/projects/esp-sr/zh_CN/latest/esp32/wake_word_engine/README.html
3. ESP-DL 快速入门（"ESP32-C3 仅适用于无需 PSRAM 的应用"）
   https://docs.espressif.com/projects/esp-dl/zh_CN/release-v1.1/esp32/get-started.html
4. ESP32-C3 技术参考手册 / 数据手册（400KB SRAM、RV32IMC、无 PSRAM、BLE5 LE-only）
   https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32c3/
5. ESP-IDF 编程指南（构建系统、分区表、OTA、FreeRTOS、性能分析）
   https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32c3/
6. `esp-tflite-micro` 组件 https://components.espressif.com/components/espressif/esp-tflite-micro

---

## 附：D1 开工前 30 秒检查

```powershell
$HOME\esp\esp-idf\export.ps1
idf.py --version                    # 期望 v5.x
python --version                    # 期望 3.8+
New-Item -ItemType Directory E:\c3-lab -Force; cd E:\c3-lab; git init
```
