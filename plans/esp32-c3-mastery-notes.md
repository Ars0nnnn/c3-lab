# 附册：112 天计划的论证材料

> 本文件是 `esp32-c3-mastery.md`（**v2.0：S3 主线 + ESP32-CAM**）的**支撑材料**，不是计划本体。
> 计划里出现的每个"为什么这么排"，答案在这里。开工时不必读；被质疑、被面试问到时回来查。

---

## A. 起点与终点

| | 现在 | 112 天后 |
|---|---|---|
| IDF | 刚装好，只会 `idf.py flash` | 能从零建带自定义分区表 + OTA + 单测 + CI 的工程 |
| 驱动 | Arduino 调库 | **能读数据手册手写驱动**，逻辑分析仪验证时序 |
| RTOS | 概念听过 | 能讲清调度/同步/优先级反转/绑核，并用测量数据证明 |
| 底层 | 黑盒 | 讲清 ROM→bootloader→app_main 全链路（含 CPU1 何时起来），读链接脚本与 map |
| AI | 会用 PyTorch 跑 demo | S3 上完整 TinyML 流水线（PSRAM arena / PIE 加速），且说清能力边界 |
| 调试 | print 大法 | JTAG 源码级调试 + coredump 现场还原 + 周期计数 profiling |
| 求职 | 无作品 | 1 个可演示多节点系统 + 16 篇技术博客 + 可量化简历项 |

---

## B. 已核实的硬件约束（决定计划边界，不是猜测）

| 约束 | 数值 | 计划中的后果 |
|---|---|---|
| **CPU** | **双核 Xtensa LX7 @240MHz**（S3） | SMP 可用：绑核、自旋锁、跨核 IPC 都是**真实验**（D85），不是纸上谈兵；W7 每个 RTOS 实验都要顺带问"另一个核呢" |
| **SRAM** | **512KB 内部 SRAM** | 内部 RAM 依然稀缺：DMA 缓冲、中断栈、热点数据优先放这里（D22） |
| **PSRAM** | **8MB 八线（N8R8）** | 摄像头帧缓存、TinyML arena、大 buffer 都靠它；**但走 cache** → DMA 直通有限制，这是 S3 上最阴的坑（D22） |
| **Flash** | **8MB** | 分区表宽松：双 OTA 各 1.5MB + storage 1MB + model 3MB（D6） |
| **SIMD** | **128 位 PIE / SIMD** | `esp-nn` 真加速 → 推理延迟"裁模型 + 用指令"两条腿，D69 必须量化收益 |
| **无线** | WiFi + **BLE5 (LE only)** | 无 BT Classic；BLE Mesh / ESP-NOW 可用（D89 用 ESP-NOW 连 CAM） |
| **串口** | **USB-Serial-JTAG + USB-OTG 两个口** | Console 必须指向实际插的那个口，否则"没日志"（D3） |
| **副板 ESP32-CAM** | ESP32-S 双核 LX6 + OV2640 + PSRAM + microSD，**无板载 USB 转串口** | 摄像头链路：3.3V USB-TTL + GPIO0 接地烧录；引脚被摄像头/PSRAM/SD 吃掉，只剩 1/3/12/13/15 可用（D18） |

## C. 关键事实裁决（决定了 AI 与视觉阶段怎么走）

| 事实 | 来源 | 计划含义 |
|---|---|---|
| `WakeNet` 系列在 S3 上跑**满血模型**（不必用 C3 的 WakeNet9s 降本版） | ESP-SR 组件 README | 唤醒词/命令词直接在 S3 做 → D74 |
| ESP-DL 2.x 主攻 S3/P4，且有 PIE/SIMD 加速 | ESP-DL 快速入门 | S3 上视觉推理可行；但**摄像头在 ESP32-CAM 上**，S3 承担"接收帧 + 决策 / 卸载"→ D86/D90 |
| TFLite Micro + INT8 + PSRAM arena 在 S3 上余量充足 | 多份实测 | 模型上限从 C3 的 40KB 放宽到 ~128KB；arena 可进 PSRAM，代价是延迟（D67–D69） |
| 经典 ESP32 的 DVP 采集走 I2S 并口、S3 走 LCD_CAM；`esp32-camera` 两套后端**同一份 API** | `esp32-camera` 文档 | 摄像头代码在 CAM 上写一次，S3 将来接 OV2640 模块也能复用逻辑 → D18 笔记写清"用的哪个后端" |

> **一句话结论**：**S3 = 主控 / 决策 / TinyML / 网关**；**ESP32-CAM = 眼睛（OV2640 采集 + 轻量检测）**。这套分工写进架构文档（D90），是真实产品的常见形态，面试时反而是加分项。

---

## D. 一条工程血脉，不是 112 个孤立 demo

```
P0 打通链路  →  P1 驱动层  →  P2 通信层  →  P3 系统层
                                              ↓
P7 求职产出  ←  P6 边缘计算  ←  P5 TinyML  ←  P4 底层架构
```

每个阶段产出的组件（`frame_proto` / `cfg` / `sht30` / `prof` / `bus` / `cam_service` …）**全部进 `common_components/`**，被后续工程引用。到 D112，手上是一个**组件库 + 一条产品线的演进史**，而不是一堆散装工程。

---

## E. 硬件与工具清单

### E.1 主控

| 板子 | 规格要求 | 用途 | 参考价 |
|---|---|---|---|
| **ESP32-S3-DevKitC-1 (WROOM-1 N8R8)** | **8MB Flash + 8MB PSRAM** | **主线全部实验**（双核/SMP/TinyML/网关） | ¥40–70 |
| **ESP32-CAM (AI-Thinker)** | ESP32-S 双核 + OV2640 + PSRAM + microSD，**无 USB 转串口** | 摄像头与视觉链路（D18–D19 / D86 / D89–D90） | ¥20–35 |
| **3.3V USB-TTL (CH340 / CP2102)** | 3.3V 电平 | **必需**：ESP32-CAM 烧录与看日志 | ¥5–10 |

### E.2 传感器与执行器（按阶段到货即可）

| 模块 | 接口 | 首次用到 | 用途 |
|---|---|---|---|
| SHT30 或 BME280 | I2C | D12 | 温湿度驱动 + 校准精度实验 |
| MPU6050 | I2C | D64 | TinyML 手势数据集 |
| 0.96" SSD1306 OLED | I2C | D15 | 自写显示驱动 |
| WS2812 灯环 (8 灯) | 单线 | D17 | RMT 时序驱动 |
| **OV2640（已焊在 ESP32-CAM 上）** | **DVP + SCCB** | **D18** | 摄像头 bring-up、性能矩阵、视觉链路 |
| INMP441 | I2S | D72 | 音频/关键词识别 |
| 按键 ×3、LED、电位器、电阻包 | GPIO/ADC | D8 | 基础外设 |
| 5V 电源 + 470Ω/1kΩ 电阻 | | D23 | 功耗测量、上拉计算 |

### E.3 仪器（面试常被问"你怎么验证的"，这些是底气）

| 工具 | 必要性 | 用途 | 参考价 |
|---|---|---|---|
| USB 电流/电压表（带 mAh 累计） | **必需** | 功耗曲线、续航实测（D23/D95）；µA 级深睡要另配万用表 | ¥20–40 |
| 24MHz 8 通道逻辑分析仪（克隆版） | **必需** | I2C/SPI/WS2812/DVP 时序、中断延迟测量 | ¥25–50 |
| 万用表 | **必需** | 电压/通断/电流交叉验证 | ¥30–80 |
| 示波器（学校实验室借） | 可选 | PWM 波形、电源纹波、抖动 | — |
| JTAG | **S3 内置，无需购买** | D48 源码级调试 | ¥0 |

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
| 2 | 能读芯片手册手写任意外设驱动（含摄像头这类有专用外设的），并用逻辑分析仪验证时序 | D8–D19 | 给一个没做过的传感器（如 BH1750）当天写出来 |
| 3 | 能诊断任意崩溃：panic / coredump / WDT / 越界 / 栈溢出 → 定位到源码行 | D5, D24, D60 | 给一段坏代码盲定位 |
| 4 | 讲清上电到 `app_main` 全链路（含 CPU1 何时被拉起）；读链接脚本与 map；能预测变量落哪个段 | D50–D56 | 白板画图 + 现场预测验证 |
| 5 | 用测量数据（周期/延迟分布/功耗）驱动优化，而不是靠猜 | D47, D57–D59, D88 | 每个优化结论都能拿出数据 |
| 6 | S3 上完整 TinyML 流水线，并说清 PSRAM / PIE 的能力边界 | D64–D77 | 现场换一个新传感器重做一遍 |
| 7 | 设计并交付多节点、断网可用、可 OTA 的边缘系统 | D78–D98 | 故障注入 10/10 通过 |
| 8 | 能在 MCU 上实现并整定一个稳定闭环，讲清定时抖动/积分饱和/微分噪声 | D78–D81 | 阶跃响应达标（超调 <10%，稳态误差 <1%） |
| 9 | 讲清 SMP 双核调度（绑核/自旋锁/核间通信/cache 一致性）与**异构任务卸载决策**（本地 vs 邻居 vs 云） | D85, D90 | 白板画双核任务图 + 自测数据论证 |
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
├─ 80_cam/                # D18–D19 摄像头（ESP32-CAM，set-target esp32）
├─ 85_s3/                 # D85–D91 双核 / SMP / 异构
├─ 90_flagship/           # D92–D98
│   └─ DESIGN.md
├─ common_components/     # ★ 全程复用
│   ├─ app_log/  frame_proto/  uart_pipe/  sht30/
│   ├─ ssd1306/  ws2812/  cfg/  mqtt_client/
│   ├─ net_check/  prof/  bus/  espnow_link/  pid/
│   ├─ cam_service/  smp_probe/
├─ tools/                 # flash.ps1 / test_e2e.py / dataset_tool.py
└─ notes/                 # 每日 Dxxx.md + arch-report.md + tinyml-selection.md + interview.md
```

> 注：仓库目录名仍叫 `c3-lab`（历史命名），硬件已换成 S3 + CAM；要不要改名是独立决定，改名前先确认 `.vscode/` 与 CI 里的路径引用。

## H. 官方资源复用（不要造轮子，但要"抄完能讲清"）

| 用途 | 路径 |
|---|---|
| 全阶段示例库 | `$IDF_PATH/examples/`（get-started / peripherals / wifi / bluetooth / protocols / system / storage / provisioning） |
| 组件注册表（一行引依赖） | `main/idf_component.yml` → `espressif/esp-tflite-micro`、`espressif/esp-sr`、`espressif/esp-dl`、`espressif/esp32-camera` |
| 构建系统 | `$IDF_PATH/tools/cmake/project.cmake`、`components/*/CMakeLists.txt` |
| 启动链源码 | `components/bootloader/bootloader_start.c`、`components/esp_system/port/cpu_start.c`（**CPU1 在 `start_other_core` 里被拉起**）、`components/esp_system/app_startup.c` |
| 链接脚本 | `components/esp_system/ld/esp32s3/sections.ld.in`、`memory.ld.in` |
| FreeRTOS 内核 | `components/freertos/FreeRTOS-Kernel/tasks.c`、`queue.c`、`portable/xtensa/port.c` |
| 测试框架 | `components/unity`、`pytest-embedded`（`idf.py -T`） |
| 目标芯片手册 | ESP32-S3 技术参考手册（Memory Map / 中断异常 / Cache / PSRAM）、ESP32-S3 数据手册、OV2640 数据手册 |

---

## I. Verification：怎么证明这 112 天没白学

| 检查点 | 时机 | 通过标准 |
|---|---|---|
| **周 Gate** | 每周日 | 当日 🎯 全过；本周组件能被外部工程引用 |
| **阶段 Gate P0** | D7 | 删 build + sdkconfig 后一条命令复现 |
| **阶段 Gate P1** | D28 | `node_v01` 连跑 2h 无泄漏，可交付他人 |
| **阶段 Gate P2** | D42 | 断电 50 次 / 断网 20 次 / OTA 失败 5 次全部自恢复 |
| **阶段 Gate P3** | D49 | 10 个 RTOS 面试题能用自测数据答（含绑核与跨核延迟） |
| **阶段 Gate P4** | D63 | 10 页底层报告完成，能预测变量落段 3/3 |
| **阶段 Gate P5** | D77 | 3 个模型全板载可跑 + 能力边界报告 |
| **阶段 Gate P6** | D91 | S3 ↔ ESP32-CAM 稳定互通 1h；双核加速比与卸载决策有量化结论 |
| **最终 Gate** | D98 | 故障注入 10/10 通过；干净 clone 一条命令构建 |
| **求职 Gate** | D105 | 简历 3 条 STAR 有量化指标；八股自测 ≥80%；3min 讲清架构 |

---

## J. 风险与应对

| 风险 | 概率 | 应对 |
|---|---|---|
| 每日 2h 不够，普遍延期 | **高** | 16 个周日全是缓冲；☆ 任务可弃；每阶段末有 Gate 但允许"降级通过"（少做 1 个延伸任务） |
| 硬件缺失（USB-TTL 未到 / 摄像头坏） | 中 | **USB-TTL 是 D18 的硬前置**：先用 S3 跑完 D1–D17；摄像头两天顺延到周日缓冲，不阻塞 D20 组件化 |
| TinyML 效果不达标 | 中 | ① 改用 Edge Impulse 现成流程 ② 退化到"二分类 + 阈值" ③ 把"做不出来的原因分析"写成博客（面试同样加分） |
| ESP-SR 唤醒词效果不达标 | 低-中 | S3 支持满血 WakeNet；仍有问题就退回 D73 自训 KWS，并把原因分析写成博客 |
| 摄像头链路卡住（花屏 / 全黑 / OOM） | **中高** | 按 D18 的坑清单逐条排除：供电 → GPIO0/XCLK → SPIRAM 是否真开 → `fb_count` → 引脚冲突；两天做不完就砍 D19 的推理部分，只留性能矩阵 |
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

### K.1 v2.0（S3 基线 + 摄像头提前）的搬运记录

| 变更 | 去向 / 理由 |
|---|---|
| 原 D18「ADC 与校准」、原 D19「三种定时器对决」 | **后移到 W13 的 D87–D88**：这两组测量的价值在"对照"，放到双核 + PSRAM 环境里做更有说服力 |
| 原 D87「PSRAM + 摄像头流水线」 | **拆解**：PSRAM → D22 内存模型；摄像头 → **D18–D19（提前）** |
| 原 D88「视觉推理单核 vs 双核」 | 基线在 D19 立；拆核对比 → **D86** |
| 原 D85「S3 上手与单/双核差异」 | **删除**：S3 是主线板，D1 就在用 |
| W12 的定时抖动基线 | 原引 D19，现改为 **D79 / D81 现场自测**（控制回路本身就是最好的抖动测量对象） |

---

## L. 参考来源（已核实）

1. ESP-IDF 编程指南（**ESP32-S3 版**：构建系统、分区表、OTA、FreeRTOS/SMP、PSRAM、性能分析）
   https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/
2. ESP32-S3 技术参考手册（Memory Map / 中断与异常 / Cache / PSRAM 章节）
   https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_cn.pdf
3. `esp32-camera` 组件（OV2640；经典 ESP32 走 I2S 并口、S3 走 LCD_CAM）
   https://components.espressif.com/components/espressif/esp32-camera
   https://github.com/espressif/esp32-camera/blob/master/README.md
4. ESP32-CAM 烧录方式（无板载转串口，GPIO0 接地进下载模式）
   https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/boot-mode-selection.html
5. ESP-SR 组件 README（WakeNet 芯片支持矩阵；S3 满血模型）
   https://components.espressif.com/components/espressif/esp-sr
6. `esp-tflite-micro` 组件 https://components.espressif.com/components/espressif/esp-tflite-micro
