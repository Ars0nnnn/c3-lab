# 附册：112 天计划的论证材料

> 本文件是 `esp32-c3-mastery.md` 的**支撑材料**，不是计划本体。
> 计划里出现的每个"为什么这么排"，答案在这里。开工时不必读；被质疑、被面试问到时回来查。

---

## A. 起点与终点

| | 现在 | 112 天后 |
|---|---|---|
| IDF | 刚装好，只会 `idf.py flash` | 能从零建带自定义分区表 + OTA + 单测 + CI 的工程 |
| 驱动 | Arduino 调库 | **能读数据手册手写驱动**，逻辑分析仪验证时序 |
| RTOS | 概念听过 | 能讲清调度/同步/优先级反转，并用测量数据证明 |
| 底层 | 黑盒 | 讲清 ROM→bootloader→app_main 全链路，读链接脚本与 map |
| AI | 会用 PyTorch 跑 demo | C3 上完整 TinyML 流水线，且说清能力边界 |
| 调试 | print 大法 | JTAG 源码级调试 + coredump 现场还原 + 周期计数 profiling |
| 求职 | 无作品 | 1 个可演示多节点系统 + 16 篇技术博客 + 可量化简历项 |

---

## B. 已核实的硬件约束（决定计划边界，不是猜测）

| 约束 | 数值 | 计划中的后果 |
|---|---|---|
| CPU | 单核 RISC-V **RV32IMC** @160MHz | **C3 无 SMP**：核相关代码在这颗芯片上永不触发 → C3 的实时性只能靠优先级/中断；**双核调度放到 S3（D86）** |
| **CPU(S3)** | **双核 Xtensa LX7** + PSRAM + PIE/SIMD | 双核调度、SMP 锁、视觉、异构卸载的落点 → 见 W13 |
| SRAM | **400KB，无 PSRAM** | Tensor arena 上限 ~100–150KB；模型必须 INT8 且 ≤40KB 才安全 |
| Flash | 典型 **4MB** | 分区表是硬约束：三 app 分区后单 app 只有 ~768KB |
| SIMD | **无 PIE / 无 AI 指令** | `esp-nn` 在 C3 上无加速；推理延迟靠裁剪模型，不靠指令 |
| 无线 | WiFi4 + **BLE5 (LE only)** | 无 BT Classic；BLE Mesh / ESP-NOW 可用 |
| 串口 | 内置 USB Serial/JTAG | 省 USB-TTL，但 Console 重定向必须配置，否则"没日志" |

## C. 关键事实裁决（决定了 AI 阶段怎么走）

| 事实 | 来源 | 计划含义 |
|---|---|---|
| `WakeNet9s` / `WakeNet10s` **明确支持 ESP32-C3**（免 PSRAM、免 SIMD 的降本版） | ESP-SR 组件 README | 唤醒词/命令词在 C3 本机可跑 → 保留为 D74 |
| ESP-DL 文档虽列 C3，但注记"仅适用于无需 PSRAM 的应用"，2.x 实际主攻 S3/P4 | ESP-DL 快速入门 | **C3 上放弃 ESP-DL 视觉**；人脸检测挪到 S3（D88） |
| TFLite Micro + INT8 20–80KB 模型在 C3 上可实时 | 多份实测 | C3 走"传感器时序 + 音频关键字"路线 |

> **一句话结论**：C3 = 传感 + 音频 TinyML + 低功耗边缘节点；S3 = 视觉 + 唤醒词 + 网关。两块板分工写进架构文档（D90），这不是妥协，是**真实产品中的异构设计**，面试时反而是加分项。

---

## D. 一条工程血脉，不是 112 个孤立 demo

```
P0 打通链路  →  P1 驱动层  →  P2 通信层  →  P3 系统层
                                              ↓
P7 求职产出  ←  P6 边缘计算  ←  P5 TinyML  ←  P4 底层架构
```

每个阶段产出的组件（`frame_proto` / `cfg` / `sht30` / `prof` / `bus` …）**全部进 `common_components/`**，被后续工程引用。到 D112，手上是一个**组件库 + 一条产品线的演进史**，而不是一堆散装工程。

---

## E. 硬件与工具清单

### E.1 主控

| 板子 | 规格要求 | 用途 | 参考价 |
|---|---|---|---|
| ESP32-C3-DevKitM-1 或 C3 SuperMini | 4MB Flash | 主线全部实验 | ¥15–40 |
| ESP32-S3-DevKitC-1 (WROOM-1 N8R8) | **8MB Flash + 8MB PSRAM** | D85–D91 视觉/异构 | ¥40–70 |

### E.2 传感器与执行器（按阶段到货即可）

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

### E.3 仪器（面试常被问"你怎么验证的"，这些是底气）

| 工具 | 必要性 | 用途 | 参考价 |
|---|---|---|---|
| USB 电流/电压表（带 mAh 累计） | **必需** | 功耗曲线、续航实测（D23/D95） |
| 24MHz 8 通道逻辑分析仪（克隆版） | **必需** | I2C/SPI/WS2812/中断延迟测量 |
| 万用表 | **必需** | 电压/通断/电流交叉验证 |
| 示波器（学校实验室借） | 可选 | PWM 波形、电源纹波、抖动 |
| JTAG | **C3/S3 内置，无需购买** | D48 源码级调试 |

### E.4 PID 闭环增购件（W12 用）

| 模块 | 接口 | 天 | 用途 | 参考价 |
|---|---|---|---|---|
| 5V PTC 陶瓷加热片（20W）+ 小铝块 | — | D78 | **主选被控对象**：热惯性大，Kp/Ki/Kd 三参数都起作用 | ¥10 |
| NTC 10K 热敏电阻 + 10K 电阻 | ADC | D78 | 快速温度反馈（比 DS18B20 快一个数量级，需 Steinhart-Hart 或查表） | ¥3 |
| MOS 低边开关模块（IRLZ44N / AO3400） | GPIO（LEDC PWM） | D78 | 调节加热功率 | ¥5 |
| 12V 2A 电源适配器 | — | D78 | 加热功率源 | ¥15 |
| **限温开关（KSD9700 75℃）+ 5A 保险丝** | — | D78 | **硬件级安全，不可省** | ¥5 |
| ☆ 直流减速电机 + AB 相霍尔编码器 | GPIO 中断 | D81 | **进阶被控对象**：转速闭环，响应快，专治"定时抖动" | ¥25 |
| ☆ TB6612 / DRV8833 电机驱动 | PWM + GPIO | D81 | 调速与正反转 | ¥8 |

> 只买前 5 项即可开工（≈¥40）；电机那套是 D81 的 ☆ 延伸，但面试更值钱：**温度环考抗积分饱和，转速环考控制周期确定性**。

---

## F. "精通"的 10 条验收标准

D106 用这张表自评（1–5 分，低于 4 分的进 D107–D109 补强）。

| # | 标准 | 对应天 | 判定方式 |
|---|---|---|---|
| 1 | 不看教程从零建一个带自定义分区表 + OTA + 单测的 IDF 工程 | D1–D42 | 现场 2h 内做完 |
| 2 | 能读芯片手册手写任意外设驱动，并用逻辑分析仪验证时序 | D8–D21 | 给一个没做过的传感器（如 BH1750）当天写出来 |
| 3 | 能诊断任意崩溃：panic / coredump / WDT / 越界 / 栈溢出 → 定位到源码行 | D5, D24, D60 | 给一段坏代码盲定位 |
| 4 | 讲清上电到 `app_main` 全链路；读链接脚本与 map；能预测变量落哪个段 | D50–D56 | 白板画图 + 现场预测验证 |
| 5 | 用测量数据（周期/延迟分布/功耗）驱动优化，而不是靠猜 | D19, D47, D57–D59 | 每个优化结论都能拿出数据 |
| 6 | C3 上完整 TinyML 流水线，并说清能力边界 | D64–D77 | 现场换一个新传感器重做一遍 |
| 7 | 设计并交付多节点、断网可用、可 OTA 的边缘系统 | D78–D98 | 故障注入 10/10 通过 |
| 8 | 能在 MCU 上实现并整定一个稳定闭环，讲清定时抖动/积分饱和/微分噪声 | D78–D81 | 阶跃响应达标（超调 <10%，稳态误差 <1%） |
| 9 | 讲清 SMP 双核调度与**异构任务卸载决策**（本地 vs 邻居 vs 云） | D86, D90 | 白板画双核任务图 + 自测数据论证 |
| 10 | 能写成文档讲给别人听，面试 3 分钟讲清架构与取舍 | D100–D105 | 模拟面试通过 |

---

## G. 仓库结构

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
│   ├─ net_check/  prof/  bus/  espnow_link/  pid/
├─ tools/                 # flash.ps1 / test_e2e.py / dataset_tool.py
└─ notes/                 # 每日 Dxxx.md + arch-report.md + tinyml-selection.md + interview.md
```

## H. 官方资源复用（不要造轮子，但要"抄完能讲清"）

| 用途 | 路径 |
|---|---|
| 全阶段示例库 | `$IDF_PATH/examples/`（get-started / peripherals / wifi / bluetooth / protocols / system / storage / provisioning） |
| 组件注册表（一行引依赖） | `main/idf_component.yml` → `espressif/esp-tflite-micro`、`espressif/esp-sr`、`espressif/esp-dl`、`espressif/esp32-camera` |
| 构建系统 | `$IDF_PATH/tools/cmake/project.cmake`、`components/*/CMakeLists.txt` |
| 启动链源码 | `components/bootloader/bootloader_start.c`、`components/esp_system/port/cpu_start.c`、`components/esp_system/app_startup.c` |
| 链接脚本 | `components/esp_system/ld/esp32c3/sections.ld.in`、`memory.ld.in` |
| FreeRTOS 内核 | `components/freertos/FreeRTOS-Kernel/tasks.c`、`queue.c`、`portable/riscv/port.c` |
| 测试框架 | `components/unity`、`pytest-embedded`（`idf.py -T`） |
| 目标芯片手册 | ESP32-C3 技术参考手册（Memory Map / Interrupt / Cache 章节）、数据手册 |

---

## I. Verification：怎么证明这 112 天没白学

| 检查点 | 时机 | 通过标准 |
|---|---|---|
| **周 Gate** | 每周日 | 当日 🎯 全过；本周组件能被外部工程引用 |
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

## J. 风险与应对

| 风险 | 概率 | 应对 |
|---|---|---|
| 每日 2h 不够，普遍延期 | **高** | 16 个周日全是缓冲；☆ 任务可弃；每阶段末有 Gate 但允许"降级通过"（少做 1 个延伸任务） |
| 硬件到货慢 | 中 | 到货前跳过对应天，先做 D50+ 的纯软件底层章节 |
| TinyML 效果不达标 | 中 | ① 改用 Edge Impulse 现成流程 ② 退化到"二分类 + 阈值" ③ 把"做不出来的原因分析"写成博客（面试同样加分） |
| C3 上 ESP-SR 跑不起来 | 低-中 | 已验证 `WakeNet9s/10s` 支持 C3；若仍失败，退回 D73 自训 KWS |
| 被八股分散注意力，工程烂尾 | **高** | 八股只在 D102 集中处理；其余时间**禁止刷题** |
| 陷入"完美主义重构" | 中 | 组件只允许在当前阶段内重构；过 Gate 后冻结接口，只加不改 |

---

## K. 内容搬家记录

原计划的以下内容按"去论证、留计划"原则迁移，主题无缩水：

| 原位置 | 去向 |
|---|---|
| 1.1 起点终点对照 | 本文件 A |
| 1.2 硬件约束 | 本文件 B |
| 1.3 事实裁决 | 本文件 C |
| 2.1 阶段血脉 | 本文件 D |
| 2.2 每日四段式 | **升级为正文每天条目的格式**（读/写/验/记） |
| 2.3 记号约定 | 正文表头一行 |
| 2.4 周日缓冲 | 正文表头一行（保留，是唯一抗延期机制） |
| 三 硬件清单 | 本文件 E |
| 六 精通 10 条 | 本文件 F |
| 七 仓库结构 | 本文件 G、H |
| 八 Verification | 本文件 I（各周 Gate 已并入正文周日条目） |
| 九 风险 | 本文件 J |
| 十 参考来源 | 本文件 L |
| W12「端云数据分级」 | 并入 D90 异构卸载 |
| W12「功耗预算模型」 | 并入 D95 功耗打磨 |

---

## L. 参考来源（已核实）

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
