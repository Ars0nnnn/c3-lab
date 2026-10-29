# PLAN：ESP32-C3 从入门到精通 · 112 天

> **周期** 16 周 = 112 天 · 每天 2h（≈224h） · 周一–周六任务日，周日复盘日
> **硬件** C3（4MB Flash / 400KB SRAM / 无 PSRAM）为主，S3（双核 LX7 / 8MB PSRAM）仅 D85–D91、D88 视觉使用
> **起点** Arduino + STM32，无 RTOS / 无 IDF 经验
> **记号** ★ 核心不可跳 · ☆ 延伸可推迟到周日 · ⚠️ 高频踩坑，当天必记 · 🎯 验收（**没有数据不算过**）
> **每天 = 读 30m / 写 75m / 验 10m / 记 5m**：读到 3 个疑问再动手；代码手敲不整贴；验必须有测量；`notes/Dxxx.md` 写三行（卡点 / 根因 / 明日风险），次日开工先读。
> **周日三选一**（写博客 / 补欠账 / 休息）。16 个周日 = 唯一的抗延期缓冲，不许当学习日。
> 论证、硬件清单、验收标准、参考来源见 `plans/esp32-c3-mastery-notes.md`。

---

## 一、阶段总览

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

> 详细版格式：**W1–W4（D1–D28）逐日展开**（前四周定工程骨架，颗粒度必须细）；**W5 起每行一天**，只保留主题 / 产出 / 验收。

---

## 二、逐日计划

### W1 · D1–D7 ｜ P0 打通链路

**周目标**：建立"删掉 `build/`、删掉 `sdkconfig`，一条命令就能复现"的工程基线。

**D1 周一 · ★ 仓库与环境基线**
- **读 30m**：IDF 快速入门 + 构建系统概览。只抓三层：`project.cmake` → 组件 `CMakeLists.txt` → 工具链；读完写 3 个疑问（`idf.py` 怎么找到组件？`sdkconfig` 从哪来？`build/` 里每个文件是谁生成的？）
- **写 75m**：① `export.ps1` 固化 `IDF_PATH`/PATH，写进 PowerShell profile 或 `tools/env.ps1`；② `git init` + 用 `idf.py create-project` 生成的 `.gitignore`（必须含 `build/`、`sdkconfig`、`sdkconfig.old`、`managed_components/`；`dependencies.lock` 要提交）；③ `00_bringup/hello` → `set-target esp32c3`；④ `tools/flash.ps1 -Port COM5 [-Clean] [-Monitor]`，内部 = `idf.py build` + `flash` + `monitor`；⑤ `README.md` 只写版本、一条命令、排障三条。
- **验 10m**：🎯 `idf.py --version` = v5.x；`git log` ≥3 个语义化 commit；`flash.ps1 -Port COM5` 首次即通
- **坑**：Windows 上 PATH 里常有两套 `idf.py`/python，先 `where.exe idf.py` 确认唯一；`sdkconfig` 不进 git（否则跨机器 diff 爆炸）；`flash.ps1` 禁止硬编码 COM 口
- **提交**：`chore: init repo + env` / `feat: sdkconfig.defaults + flash.ps1` / `docs: README quickstart`

**D2 周二 · ★ 烧录三件套**
- **读 30m**：Bootloader 概述 + 分区表概述 + `esptool.py write_flash` 参数表。带着问题：为什么 app 偏移不是固定的 0x10000？
- **写 75m**：`build/` 里找 `flash_args`，照它手工烧：`bootloader.bin@0x0` → `partition-table.bin@0x8000` → `hello.bin@0x10000`（含 `--flash_mode dio --flash_freq 80m --flash_size 4MB`）；再故意把 app 烧到 `0x0`
- **验 10m**：🎯 笔记画出三个 bin 的偏移 / 大小 / 作用；关掉 `flash.ps1` 手工烧也能点亮；记下烧错偏移时的原始报错
- **坑**：烧错偏移 → 重启循环 `invalid header: 0xffffffff`；app 偏移由分区表决定，换 csv 就变；未 `erase_flash` 时旧分区表残留会骗你半小时
- **提交**：`docs: flash offsets & three bins` + `00_bringup/manual_flash.ps1`

**D3 周三 · ★ menuconfig 与配置固化 ⚠️**
- **读 30m**：Kconfig 机制 —— `Kconfig.projbuild` / `sdkconfig.defaults` / `sdkconfig` 三者关系，`CONFIG_` 前缀怎么进代码
- **写 75m**：menuconfig 改五项：Console → **USB Serial/JTAG**、Flash 4MB@80MHz、优化 `-Og`、assert 开启、默认日志级 Info；`idf.py save-defconfig` 生成 `sdkconfig.defaults`，**手工删掉所有等于默认值的行**
- **验 10m**：🎯 删 `sdkconfig` 重 build → `diff` 与旧 `sdkconfig` 逐行一致；USB 口直连 `idf.py monitor` 有日志
- **坑**：Console 改成 USB-JTAG 后，若板子 USB 实际接 CH340 桥 → 直接"没日志"；menuconfig 改了但没落进 defaults 是全计划最大隐患；defaults 里写了默认值行，未来 IDF 升级必冲突
- **提交**：`feat: sdkconfig.defaults (usb-jtag console, 4MB, -Og)`

**D4 周四 · 日志与错误处理**
- **读 30m**：`esp_log.h`（等级、`LOG_LOCAL_LEVEL`（编译期）、`esp_log_level_set`（运行期））+ `esp_err.h`（`ESP_ERROR_CHECK` 展开成什么、`ESP_RETURN_ON_ERROR` 家族）
- **写 75m**：`components/app_log`：TAG 命名规则（`模块名`≤16 字符）、`CHECK_ERR`/`LOG_ERR` 宏（打印 `file:line` + `esp_err_to_name`）、模块 ID 枚举、运行期按 TAG 调等级
- **验 10m**：🎯 5 个等级运行期动态开关生效；把 `app_log` 用 path 依赖引到另一个工程可编可跑
- **坑**：`LOG_LOCAL_LEVEL` 是编译期，想动态必须 `esp_log_level_set`；`ESP_ERROR_CHECK` 不返回，不能当错误处理；TAG 过长会被截断
- **提交**：`feat: app_log component (module tags + runtime levels)`

**D5 周五 · ★ 崩溃现场解析 ⚠️**
- **读 30m**：Fatal Errors 文档 —— Guru Meditation 各行含义、Backtrace 怎么解、`idf.py monitor` 的自动解码
- **写 75m**：一个工程 5 个 `#if CASE_x` 分支：① 空指针读 ② 函数指针跳垃圾地址 ③ 除零 ④ `assert` ⑤ 递归/大数组栈溢出
- **验 10m**：🎯 每类都用 `addr2line -e build/*.elf 0x...` 定位到**自己源码行号**；`notes/D005` 存 5 段原始日志 + 解码结果
- **坑**：**RISC-V 除零不 trap**（和 Cortex-M 的 UsageFault 不同），会静默返回，必须软件检查 —— 这条本身就是面试题；`-Og` 仍会内联，栈帧可能不准；release 无符号时 backtrace 无意义
- **提交**：`feat: crash lab (5 cases)` + `docs: decode cheatsheet`

**D6 周六 · ★ 自定义分区表**
- **读 30m**：Partition Tables 文档 —— csv 字段、app 分区 64KB 对齐、`otadata` 两个扇区的用途、nvs 默认位置
- **写 75m**：先算账再写 csv。4MB 的可用空间从 `0x20000` 起：三 app 各 `0xC0000` + storage `0x80000` + model `0xC0000` = `0x3A0000`，剩 `0x60000` 留白。落地 `partitions.csv`：`nvs 0x9000/0x6000` · `otadata 0xF000/0x2000` · `factory 0x20000/0xC0000` · `ota_0 0xE0000/0xC0000` · `ota_1 0x1A0000/0xC0000` · `storage 0x260000/0x80000` · `model 0x2E0000/0xC0000`；再写 `esp_partition_write/read` 往返 1KB
- **验 10m**：🎯 `idf.py partition-table` 输出与 csv 逐行吻合；写后读回**字节一致**；断电重上电数据仍在
- **坑**：4MB 装不下"三个 1M app + 1M storage + 1M model"，必须做减法 —— 这一天的真实功课是算账不是抄模板；app 分区非 64KB 对齐 build 直接报错；改了分区表必须 `erase_flash`，否则旧 nvs 残留
- **提交**：`feat: custom partitions.csv (3 app slots + storage + model)`

**D7 周日 · 复盘 + 博客 #1**
- 写《ESP-IDF 构建系统、烧录三件套与分区表》
- 🎯 **阶段 Gate P0**：能脱稿讲清 `app.bin` 从源码到烧录的完整路径；别人 clone 后一条命令 flash 成功

---

### W2 · D8–D14 ｜ P1 驱动层（一）GPIO / PWM / UART

**周目标**：不再"调库"，而是"按手册写"，每个参数都有测量依据。

**D8 周一 · ★ GPIO 与中断**
- **读 30m**：GPIO 文档 + `driver/gpio.c` 中断路径；`gpio_install_isr_service` 为什么只能装一次
- **写 75m**：3 个按键（上下拉、边沿触发）+ 每键独立 `esp_timer` 单次定时器软件消抖 + 事件队列上报任务；记录 20 次触发的时间戳
- **验 10m**：🎯 逻辑分析仪**实测抖动宽度**（min/均值/max），据此定消抖参数而不是抄 20ms；给出中断→任务收到的延迟
- **坑**：ISR 里禁止 printf/阻塞；service 全局唯一，多键靠 arg 区分；释放抖动通常比按下更长
- **提交**：`10_drivers/gpio_key` · `feat: gpio_key (isr + measured debounce)`

**D9 周二 · ★ LEDC PWM**
- **读 30m**：LEDC 文档 —— 时钟源、`duty_resolution` 与 `freq` 的互斥关系、硬件 fade
- **写 75m**：gamma 校正呼吸灯（查表）；扫 `freq∈{1k,5k,10k,20k,40k}` × `res∈{8..14}`，记录哪些组合 `ledc_timer_config` 失败；硬件 fade vs 软件循环渐变对比
- **验 10m**：🎯 产出 `freq × max_res` 表并验证 `freq ≤ 80MHz/2^res`；示波器/逻辑分析仪测占空比误差
- **坑**：14bit 时频率只剩 ~4.8kHz；C3 只有 6 通道；低占空比区非线性（LED 亮度 + 驱动），所以必须 gamma
- **提交**：`feat: ledc pwm + gamma table + freq/res matrix`

**D10 周三 · ★ UART 驱动与吞吐**
- **读 30m**：`uart_driver` 事件队列模式、DMA 与 ring buffer 的关系、`tx_buffer_size` 语义
- **写 75m**：`components/uart_pipe`：DMA 发送 + 中断接收环形缓冲 + 溢出计数 + 阻塞/非阻塞两套 API
- **验 10m**：🎯 3Mbps 连续发 1MB **零丢包**（PC 侧算 CRC32 校验）；反向收 1MB 也零丢；吞吐 Mbps 数字入笔记
- **坑**：UART0 默认是 console（D3 已迁到 USB-JTAG，这里才能复用）；DMA 描述符与 ring buffer 是两层，别重复缓冲
- **提交**：`feat: uart_pipe (dma tx + isr rx ring)`

**D11 周四 · ★ 串口协议设计 ⚠️**
- **读 30m**：COBS / HDLC / MODBUS 三种成帧思路对比；CRC16-CCITT 查表生成
- **写 75m**：`components/frame_proto`：`0xAA55` 帧头 + len + cmd + payload + CRC16 + 状态机（IDLE/HDR/LEN/PAYLOAD/CRC）+ 超时重同步 + 回调注册
- **验 10m**：🎯 注入 5 类故障（随机噪声、半帧、错 CRC、粘包两帧、len 造假超长帧）→ 全部正确重同步、不越界、不泄漏
- **坑**：**len 必须先校验上限再 malloc**，否则一个假长度就能耗尽堆；帧头出现在 payload 内的转义问题；重同步超时要小于最小帧间隔
- **提交**：`feat: frame_proto (resync state machine) + 5 fuzz cases`

**D12 周五 · ★ I2C 与 SHT30**
- **读 30m**：新 I2C master API（`i2c_new_master_bus` / `i2c_master_transmit_receive`）；SHT30 数据手册命令表 + CRC8（多项式 0x31）
- **写 75m**：`components/sht30`：单次测量 + 周期测量两种模式、CRC8 校验、按手册给够转换延时、完整错误码
- **验 10m**：🎯 与商用温湿度计对比误差 <0.5℃ / 3%RH（10 点采样）；CRC8 用官方样例向量做单测
- **坑**：CRC8 初值是 `0xFF` 不是 0；周期模式要先发命令再读、读前必须等；地址 `0x44/0x45` 由 ADDR 引脚决定
- **提交**：`feat: sht30 driver (crc8 + periodic mode)`

**D13 周六 · ★ I2C 故障与实测**
- **读 30m**：I2C 时序、时钟拉伸、仲裁；SCL 被从机拉死时的 9-clock bus recovery
- **写 75m**：故障注入 5 种（拔线 SDA 悬空、错地址、1m 长飞线、从机不 ACK、SCL 拉死）+ 超时保护 + GPIO 位翻转 9 clock 恢复
- **验 10m**：🎯 连续 100 次拔插，**总线卡死 0 次**；时序截图 3 张（正常 / 长线 / 时钟拉伸）
- **坑**：默认 `i2c` 超时可能过长，测不出"卡死"；软件 reset 不一定能救回被拉死的 SCL；4.7k 上拉在长线上波形变缓 → 需算 `t_r≈0.847RC`
- **提交**：`feat: i2c fault injection + bus recovery`

**D14 周日 · 复盘 + 博客 #2**
- 写《I2C：从时序图到可复用驱动》
- 🎯 组件能通过 `idf_component.yml` path 依赖被新工程一行引用

---

### W3 · D15–D21 ｜ P1 驱动层（二）显示 / SPI / RMT / ADC

**D15 周一 · ★ SSD1306 自写驱动 ⚠️**
- **读 30m**：SSD1306 数据手册 —— 页寻址 vs 水平寻址、GDDRAM 结构、命令表
- **写 75m**：初始化序列 + 1KB 显存缓冲 + 6x8 与 8x16 字模 + 脏页局部刷新 + `oled_printf` 风格 API
- **验 10m**：🎯 实时显示温度 + 自造 16x16 中文点阵；对比局部刷新与全屏刷新的帧率，无闪烁
- **坑**：页寻址下 Y 只能按页写；单次 I2C 传输超长要分包；初始化里的 delay 不能省（否则黑屏却无报错）
- **提交**：`components/ssd1306` · `feat: ssd1306 (page buffer + dirty refresh)`

**D16 周二 · ★ SPI 与吞吐极限**
- **读 30m**：SPI Master 事务队列、DMA、`dummy_bits` / `cs_ena_pretrans` 时序参数
- **写 75m**：MOSI↔MISO 短接做环回，40MHz 传 10KB；可配时序参数做对照实验
- **验 10m**：🎯 理论 vs 实测吞吐偏差分析（开销来自事务队列 / 中断 / 描述符，必须说出具体占比）
- **坑**：`dummy_bits` 忘配会整体错一位；DMA 缓冲必须 `MALLOC_CAP_DMA`；飞线长时 40MHz 已经不稳，波形先看再调参数
- **提交**：`feat: spi loopback benchmark`

**D17 周三 · ★ RMT 与 WS2812**
- **读 30m**：RMT 编码器 API（`rmt_new_tx_channel` / `rmt_new_bytes_encoder` / `rmt_new_copy_encoder`）
- **写 75m**：编码 T0H/T1H/Treset + 色轮 + 亮度渐变 + 8 灯流水
- **验 10m**：🎯 逻辑分析仪确认 `T0H=0.4µs±150ns`、`T1H=0.8µs±150ns`、`Treset>50µs`；连跑 100 帧无错乱
- **坑**：WS2812 对中断延迟极敏感，只有硬件 RMT 才稳（软件 bit-bang 一定花）；RMT 内存块不够会静默卡住
- **提交**：`components/ws2812` · `feat: ws2812 via rmt encoder`

**D18 周四 · ★ ADC 与校准 ⚠️**
- **读 30m**：ADC oneshot / continuous(DMA)、`adc_cali` 曲线拟合 vs 线性、衰减档与量程
- **写 75m**：电位器 10 点测量（0.1–2.5V），出 raw / cali / 万用表三方对照表；再开 WiFi 复测 **ADC2**
- **验 10m**：🎯 校准后误差 <2% FS；WiFi 开启时 ADC2 的行为有明确结论（可用 / 需关 WiFi / 完全不可用）
- **坑**：**ADC2 与 WiFi 抢硬件**，这是 C3 的硬约束不是 bug；0dB 衰减量程仅 ~1.1V；oneshot 每次要重新 config
- **提交**：`feat: adc cali benchmark + wifi/adc2 report`

**D19 周五 · ★ 三种定时器对决**
- **读 30m**：`esp_timer` / GPTimer / FreeRTOS Timer 的实现差异（中断 vs 任务上下文、精度、漂移）
- **写 75m**：同一个 1kHz 周期任务三种实现各跑 10 万次，记录抖动分布；顺手落地 `prof` 雏形（min/均值/P99/max）
- **验 10m**：🎯 抖动表 + "什么场景用哪个"的结论；量化 `configTICK_RATE_HZ=100` 对 1ms 任务的影响
- **坑**：FreeRTOS timer 精度受 tick 限制（默认 10ms）；`esp_timer` 回调在中断上下文不能阻塞；`vTaskDelay(1)` 在 1kHz 需求下必然漂移
- **提交**：`feat: timer shootout + jitter table`（`prof` 起点）

**D20 周六 · ★ 组件化与单元测试 ⚠️**
- **读 30m**：Component Manager、`idf_component.yml`、Unity、pytest-embedded
- **写 75m**：把 D8–D17 的驱动全部搬进 `components/`（各带 `idf_component.yml` + `REQUIRES`/`PRIV_REQUIRES`），纯逻辑部分抽成可测函数，写 ≥8 个 Unity 单测
- **验 10m**：🎯 `idf.py -T` 单测全绿；`idf.py fullclean` 后仍可编；外部工程 path 依赖引用成功
- **坑**：不抽纯函数就没法在 PC/linux target 测；`REQUIRES` 与 `PRIV_REQUIRES` 混用会把头文件泄漏到使用者
- **提交**：`refactor: drivers → components + 8 unit tests (green)`

**D21 周日 · 复盘 + 博客 #3**
- 写《把 demo 变成组件：ESP-IDF 组件化与单元测试》
- 🎯 **周 Gate**：组件库可被外部工程一行依赖引用

---

### W4 · D22–D28 ｜ P1 驱动层（三）内存 / 低功耗 / 可靠性 / 综合

**D22 周一 · ★ 内存模型 ⚠️**
- **读 30m**：Memory Types 文档 + `multi_heap` 分配算法
- **写 75m**：`heap_caps_malloc` 全 CAP 组合矩阵实验（INTERNAL / DMA / IRAM8 / SPIRAM）；`IRAM_ATTR` 函数对比；故意复现 `MALLOC_CAP_DMA|INTERNAL` 失败
- **验 10m**：🎯 分配矩阵表（哪个组合成功、落在哪个堆）；能解释"这个 buffer 为什么不能放外部 RAM"
- **坑**：C3 **无 PSRAM**，所有 SPIRAM 相关 CAP 必然失败 —— 这些失败正是要记录的结论；IRAM 只留很小一块给代码
- **提交**：`docs: memory cap matrix`

**D23 周二 · ★ 低功耗实测 ⚠️**
- **读 30m**：Sleep Modes 文档、RTC 定时唤醒、`ext0` GPIO 唤醒、power management 配置
- **写 75m**：active / light sleep / deep sleep 三态各测 60s 平均电流；deep sleep + 定时唤醒 + GPIO 唤醒各跑一轮
- **验 10m**：🎯 三态电流实测（USB 表 + 万用表交叉验证）；唤醒周期误差 <1%，并记录当时室温
- **坑**：USB 供电+USB 表测不出真实 µA（表分辨率与桥芯片漏电）；deep sleep 时 GPIO 保持状态与上拉；RTC 精度随温度漂移，不记温度等于没数据
- **提交**：`docs: sleep current measurements`

**D24 周三 · ★ 看门狗**
- **读 30m**：TWDT / INT_WDT / RTC_WDT 三者的监控对象与触发后果；panic 处理流程
- **写 75m**：构造 5 类 WDT 场景：任务饿死、临界区过长、ISR 卡死、`vTaskDelay` 误用、flash 操作超时
- **验 10m**：🎯 每类都能读懂 panic 信息（哪个任务没喂狗、卡在哪个函数）并写出修复
- **坑**：TWDT 默认只监控 idle 任务，**不 `esp_task_wdt_add` 自己的任务等于没开**；ISR 卡死只能靠 INT_WDT
- **提交**：`feat: wdt 5 cases + fixes`

**D25 周四 · ★★ 综合工程：采集节点 v0.1**
- **读 30m**：只读自己本周的笔记，不读新文档
- **写 75m**：SHT30 + OLED + 按键 + `frame_proto` + `esp_console` 命令行（`get/set/help/reboot`）
- **验 10m**：🎯 串口命令全可用；连跑 2h `heap_caps_get_free_size` 前后差 <1KB；WDT 零触发
- **坑**：`esp_console` 会抢 UART，必须与 `frame_proto` 分层（一行一线或时分复用）；任务栈不足表现为偶发重启，用 `uxTaskGetStackHighWaterMark` 反查
- **提交**：`10_drivers/node_v01` · `feat: node_v01 integration`

**D26 周五 · 代码审查**
- **读 30m**：IDF 编码风格 + MISRA-C 关键规则清单
- **写 75m**：按自查表过 `node_v01`：错误码、资源释放、超时、越界、整数溢出、`volatile`、魔数
- **验 10m**：🎯 找出并修复 ≥10 个**真实**问题，记录 `notes/D026-review.md`
- **坑**：自查最容易只验"能不能跑"，要找的是边界与失败路径；单核上 `volatile` 也不等于原子
- **提交**：`fix: 10 review findings`

**D27 周六 · ☆ 硬件与测量基础**
- **读 30m**：上拉电阻计算、去耦、纹波、探头接地
- **写 75m**：按 `t_r≈0.847RC` 算 I2C 上拉并实测波形；测 3.3V 纹波；长短地线探头对比
- **验 10m**：🎯 波形图 3 张 + 一页"硬件常识"笔记（面试常问）
- **坑**：长地线探头引入振铃，会把好电路误判成坏电路
- **提交**：`docs: hardware basics`

**D28 周日 · 复盘 + 博客 #4 + 机动**
- 写《低功耗实测：从 20mA 到 20µA》
- 🎯 **阶段 Gate P1**：`node_v0.1` 可交付给别人使用，连跑 2h 无泄漏

---

### W5 · D29–D35 ｜ P2 通信层（一）WiFi / HTTP / MQTT / 配置

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D29** 周一 | ★ WiFi STA 状态机 ⚠️ | `wifi_sta`：IDLE→CONNECTING→CONNECTED→BACKOFF 指数退避 + 抖动 | 拔路由器电源，30s 内自恢复且不刷屏 |
| **D30** 周二 | `esp_netif` / SNTP / DNS | `net_check`：打印 IP/掩码/网关/DNS + SNTP 同步 + 单调时钟封装 | 冷启动到拿到 IP <3s |
| **D31** 周三 | ★ HTTPS 客户端 ⚠️ | GET/POST、chunked、`esp_crt_bundle` 证书校验、超时重试 | 故意用错 CA → 请求被拒（**不是静默放行**） |
| **D32** 周四 | ★ MQTT | `mqtt_client`：QVoS 0/1/2、遗嘱、keepalive、重连、离线队列 | 断网 60s 恢复后 QoS1 消息不丢不重 |
| **D33** 周五 | 本地 Web 服务 | 内嵌配置页（表单 + JSON API + WebSocket 实时数据） | 手机浏览器改配置 → 写 NVS → 生效，全程不断电 |
| **D34** 周六 | ★ NVS 与配置中心 ⚠️ | `cfg`：默认值 / 范围校验 / **版本迁移** / 掉电安全写入 | 反复掉电 100 次配置完好；旧配置自动迁移 |
| **D35** 周日 | 复盘 + 博客 #5 | 《ESP32 联网的可靠性设计：重连、退避、离线队列》 | 博客里有自己测的重连时间分布 |

---

### W6 · D36–D42 ｜ P2 通信层（二）BLE / 配网 / OTA

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D36** 周一 | ★ NimBLE 基础 | `ble_node`：改造 `bleprph`，1 个可写特征 + 1 个 notify 特征 | nRF Connect 读写 + notify 全通 |
| **D37** 周二 | ★ BLE 参数与功耗 | conn interval 7.5ms→500ms 扫参，测吞吐与平均电流 | 吞吐-功耗曲线 + "用哪个 interval"的结论 |
| **D38** 周三 | BLE + WiFi 共存 ⚠️ | 同开 BLE 广播 + WiFi 大流量，测双方吞吐衰减 | 量化衰减百分比 + 调优后恢复多少 |
| **D39** 周四 | ★ 配网 | `prov`：SoftAP + BLE 双通道配网，落 NVS，失败可重入 | 清空 NVS → 手机配网 → 联网上报，全流程可复现 |
| **D40** 周五 | ★★ OTA 与回滚 ⚠️ | `ota`：`esp_https_ota` + 版本号/CRC 校验 + `esp_ota_mark_app_valid` | 升级中途断电 → 重启回到旧版本可用（不砖） |
| **D41** 周六 | ★★ 综合：联网节点 v1.0 | `node_v1` = node_v0.1 + 配网 + MQTT + OTA + 配置中心 | 出厂到联网上报 **5 分钟**完成，附操作手册 |
| **D42** 周日 | 复盘 + 博客 #6 | 《BLE 与 OTA：产品化的两条命脉》 | **阶段 Gate P2**：断电 50 次 / 断网 20 次 / 升级失败 5 次全部自恢复 |

---

### W7 · D43–D49 ｜ P3 系统层：FreeRTOS 内核

**周目标**：把"我知道 RTOS"变成"我能用测量数据解释 RTOS 的每个行为"。

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D43** 周一 | ★ 任务与调度器 | 优先级抢占/轮转实验；`vTaskDelay` vs `vTaskDelayUntil` 抖动对比 | 抖动数据表；能解释 tick=100Hz 下 1ms 任务的命运 |
| **D44** 周二 | ★ 同步原语选型 | 同一生产者-消费者用队列 / 流缓冲 / 任务通知三种实现 | 吞吐与延迟对比表 → 选型规则（面试高频） |
| **D45** 周三 | ★ 优先级反转 ⚠️ | 构造经典三任务反转，用带继承的 mutex 修复 | 修复前后最坏延迟数字（如 210ms → 300µs）+ 时序图 |
| **D46** 周四 | ★ 堆与栈 ⚠️ | 4 个堆 region 分配走向；`uxTaskGetStackHighWaterMark` 反推最小栈；触发溢出 | 每任务栈大小**有实测依据**；能解释 `heap_caps` 与 `malloc` 关系 |
| **D47** 周五 | ★ 中断与临界区 | GPIO 中断打点 + 逻辑分析仪测"中断→任务收到"延迟；改变临界区长度 | 中断延迟基线 + 临界区长度影响曲线 |
| **D48** 周六 | ★★ JTAG 源码级调试 ⚠️ | `idf.py openocd` + `idf.py gdb`；断点、watchpoint、看 `pxCurrentTCB` | 像调 PC 程序一样调固件；写清"打断点为什么会复位" |
| **D49** 周日 | 复盘 + 博客 #7 | 《用数据说明白：ESP32-C3 上的 RTOS 调度与延迟》 | **阶段 Gate P3**：能用自测数据答 10 个 RTOS 面试题 |

---

### W8 · D50–D56 ｜ P4 底层架构（一）启动链 / 链接 / 内存映射

**周目标**：把"上电到 `app_main`"这个黑盒完全打开。

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D50** 周一 | ★★ 启动链全景 ⚠️ | 开 bootloader 详细日志，逐行标注；定位 eFuse/strap pin 判定分支 | 启动时序图（eFuse → ROM → 2nd boot → app） |
| **D51** 周二 | ★★ app 启动细节 | 在 `call_start_cpu0`、`esp_startup_start_app`、`main_task` 插桩 | `app_main` 之前**函数级调用链 ≥12 层** |
| **D52** 周三 | ★ 链接脚本与段布局 ⚠️ | `objdump -h/-t` 列出各段，对照 `.map`；预测新变量落段 | 连续 3 次预测段位置全中 |
| **D53** 周四 | ★ 内存映射与 mmap ⚠️ | `esp_partition_mmap` 把 `model`/`storage` 分区映射后读取 | 打印映射地址窗口验证内容；解释"为什么能像内存一样读 Flash" |
| **D54** 周五 | ★ 异常与中断向量 | 注册 GPIO/UART/Timer 三类中断；读 `mtvec/mcause/mepc/mstatus` | 故意触发异常后能靠 `mcause` 判类型并手写解析 |
| **D55** 周六 | ★ RISC-V 汇编与周期计数 | 手写汇编版 `memcpy` 或 CRC16；`csrr mcycle/minstret` 做 `prof` | 汇编版比 C 版快且有周期数据；能读反汇编 |
| **D56** 周日 | 复盘 + 博客 #8 | 《ESP32-C3 从上电到 app_main：一条链走完》 | 博客含自己画的调用链图 |

---

### W9 · D57–D63 ｜ P4 底层架构（二）Cache / 性能 / 可靠性 / 安全

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D57** 周一 | ★ Cache 与 Flash 执行 ⚠️ | 热点函数加/不加 `IRAM_ATTR` 的 `mcycle` 对比；查 TRM 确认 cache 容量/路数 | 提升百分比 + "中断处理为什么要放 IRAM"的解释 |
| **D58** 周二 | ★ 性能测量方法论 | 完善 `prof`：区间计时、P50/P99 分布、跨任务统计 | 能给任意函数出耗时分布图 |
| **D59** 周三 | 二进制体积优化 | `idf.py size-*` 家族逐项裁剪：日志级别、`-Os`、LTO、关组件 | app 体积缩小 ≥15% 且功能不变（附对比表） |
| **D60** 周四 | ★ 崩溃现场还原 ⚠️ | coredump 存 flash 分区；构造崩溃后离线解析调用栈与关键变量 | 从一次真实崩溃还原任务名 + 调用栈 + 寄存器 |
| **D61** 周五 | 安全基础 | Secure Boot / Flash Encryption / eFuse 风险评估；试一次 encryption | 一页安全清单：什么场景必须开、开了什么不能回滚 |
| **D62** 周六 | ★★ 交付：底层分析报告 | 把 W8–W9 的测量整理成 10 页报告 | `notes/arch-report.md`：有图有表有数据有结论 |
| **D63** 周日 | 复盘 + 博客 #9 | 《为什么我的中断有 3µs 抖动：Cache、IRAM 与临界区》 | **阶段 Gate P4**：报告能被非嵌入式背景的人看懂一半 |

---

### W10 · D64–D70 ｜ P5 TinyML（一）数据 → 训练 → 量化 → 部署

**周目标**：把"我会 PyTorch"变成"我能让 400KB SRAM 的单片机实时跑神经网络"。

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D64** 周一 | ★★ 数据集采集 ⚠️ | C3 以 100Hz 采 MPU6050；5 类动作 × 50 次，含静止/噪声负样本 | 数据集 v1 + 采集协议、标签规则、已知偏差写明 |
| **D65** 周二 | ★ 预处理与特征 | `preprocess.py`：滑窗重叠切分、归一化、时域+频域特征 | 特征图显示类别可分 |
| **D66** 周三 | ★ PC 训练 | `train.py`：小 CNN/DSCNN + 混淆矩阵 + 交叉验证 | 验证集准确率 ≥90%；参数量 <30K |
| **D67** 周四 | ★ INT8 量化 ⚠️ | FP32 vs full-integer INT8 对比（精度/大小/tensor 类型） | 精度下降 <3%，体积 ≤40KB，输入输出均 int8 |
| **D68** 周五 | ★★ 部署到 C3 ⚠️ | 模型转 C 数组 → `MicroInterpreter` → op resolver 裁剪 → arena 调参 | `50_tinyml/gesture` 板上实时推理，端到端 <100ms |
| **D69** 周六 | ★ 性能调优 | 二分法定 arena 上限；裁 op；`esp-nn` 开关对比（C3 无 PIE，预期收益有限） | 性能基线表：延迟 / arena / CPU / 内存峰值 |
| **D70** 周日 | 复盘 + 博客 #10 | 《从 CSV 到 INT8：在 400KB SRAM 里塞进一个神经网络》 | 博客含完整流水线图与踩坑清单 |

---

### W11 · D71–D77 ｜ P5 TinyML（二）模型热更新 / 音频 / 官方方案 / 选型

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D71** 周一 | ★ 模型进分区 + mmap ⚠️ | 模型放 `model` 分区，mmap 直喂 interpreter，支持 OTA 换模型 | **不重新烧 app** 就能换模型并生效 |
| **D72** 周二 | ★ 音频链路 I2S + INMP441 ⚠️ | 24bit 采集 → 降采样 16kHz → 存 WAV / 串口回传 | PC 播放清晰无爆音；采样率误差 <1% |
| **D73** 周三 | ★★ 自训 KWS | MFCC + 小 CNN，3 类（开灯/关灯/噪声），复跑 D68 流水线 | `50_tinyml/kws` 现场 10 次识别 ≥8 次正确 |
| **D74** 周四 | ★ ESP-SR 官方方案 ⚠️ | 跑通唤醒词例程 → 接业务逻辑（唤醒后开灯/上报） | 1 小时内唤醒次数/误唤醒次数都有记录 |
| **D75** 周五 | ★ 异常检测（无监督） | 自编码器或统计阈值法，输出异常分数 + 报警 | 注入异常能报警，误报率有数据 |
| **D76** 周六 | ★★ 三模型横向选型 | 手势/关键词/异常在**精度-延迟-内存-功耗**四维对比 | `notes/tinyml-selection.md`：能回答"什么任务用哪种方案" |
| **D77** 周日 | 复盘 + 博客 #11 | 《ESP32-C3 到底能跑多大的模型：实测边界》 | **阶段 Gate P5**：3 个模型板载可跑 + 有边界结论 |

---

### W12 · D78–D84 ｜ P6 边缘计算（一）PID 闭环控制 + 规则 / 离线

**周目标**：把闭环从头到尾坐实。PID 公式三行就写完，难的是**定时抖动让 D 项变放大器、积分饱和让系统开机冲顶、传感器噪声让你以为 Kd 没用**。本周复用 D19（定时抖动）、D47（中断延迟）、D55（周期计数）的全部测量结果。

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D78** 周一 | ★★ 闭环硬件搭建与开环辨识 ⚠️ | 温控回路（加热片 + MOS + NTC + 限温开关）；0/25/50/75/100% PWM 开环阶跃；离线拟合**一阶惯性 + 滞后**参数 | 🔥 先接限温开关与保险丝再上电。拿到升温斜率、最大温升、时间常数 τ |
| **D79** 周二 | ★ PID 离散化实现 | `pid` 组件：位置式 + 增量式、抗积分饱和（clamp/back-calculation）、不完全微分、输出限幅；float vs 定点 Q15 用 `mcycle` 对比 | 两版都能跑；定点版更快且有数字；纯算法部分有单测 |
| **D80** 周三 | ★★ 整定与闭环实测 ⚠️ | 开环数据离线仿真出初值 → 上板 Z-N / 手动整定 → 测超调/上升时间/调节时间/稳态误差/振荡次数；☆ WebSocket 实时曲线 + 在线改参 | 三组参数对比表 + 曲线；终版超调 <10%、稳态误差 <1%、无持续振荡 |
| **D81** 周四 | ★★ PID 在 MCU 上的真实问题 ⚠️ | 四组破坏实验：① `vTaskDelay` vs `Until` ② 微分滤波有无 ③ 关抗饱和看冲顶 ④ 回路内插 5ms 阻塞；☆ 换电机转速环重复 | `notes/pid-mcu.md`：每种劣化都有曲线 + 数据；讲清"为什么 PI 够用还要 D" |
| **D82** 周五 | ★ 规则引擎 + 消息总线 | JSON 规则存 NVS（`if 温度>30 且持续60s then 报警`）+ 去抖 + 事件抑制；`bus` 组件（订阅/异步分发/背压） | 改规则**不重启**即生效；新增模块**不改老代码** |
| **D83** 周六 | ★★ 断网续传 + 时间对齐 ⚠️ | LittleFS 环形日志 + 断网缓存 + 恢复补传 + 幂等去重；UTC + 单调 tick + 漂移补偿 | 断网 30min 数据一条不丢不重；两节点时间对齐误差 <50ms |
| **D84** 周日 | 复盘 + 博客 #12 | 《PID 不难，难的是它跑在一颗 160MHz 单核上》 | **阶段 Gate**：闭环连续稳定 ≥2h 无振荡无冲顶；博客含三组整定曲线 |

---

### W13 · D85–D91 ｜ P6 边缘计算（二）异构：双核调度 / SMP / 视觉 / 卸载

**周目标**：C3 是单核，**双核调度必须在 S3 上练**。本周同时回答两个面试必问：「SMP 下怎么保证临界区安全」「什么时候该把任务挪到另一端」。

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D85** 周一 | ★ S3 上手与单/双核差异 | 同一份代码 `set-target esp32s3` 直接编过 | **一套组件两款芯片**都能编能跑；列出"哪些代码在 C3 上是死路" |
| **D86** 周二 | ★★ 双核调度与 SMP ⚠️ | `smp_probe`：① 绑核/不绑的吞吐延迟 ② 跨核 `portENTER_CRITICAL(&mux)` 复现数据竞争并修复 ③ `esp_ipc_call` ④ 测跨核唤醒延迟 | 单核 vs 双核加速比数字；竞争 bug 复现日志 + 修复前后差异 |
| **D87** 周三 | ★ PSRAM + 摄像头流水线 ⚠️ | 大 buffer 内部 RAM vs PSRAM 带宽延迟对比；采集 JPEG 扫分辨率/帧率/内存（帧缓存必须进 PSRAM） | 内存分层表 + "什么放 PSRAM"规则 + 帧率数据（如 QVGA 25fps） |
| **D88** 周四 | ★★ 视觉推理：单核做不动 → 双核分工 | ESP-DL 人脸检测 + 串口输出坐标；采集搬一个核、推理搬另一个核，对比单核基线 | `85_s3/face` 实测 ≥5fps；双核分工相较单核的加速比有数字 |
| **D89** 周五 | ★ 异构通信：ESP-NOW ⚠️ | C3↔S3 双向传输：ACK、重传、丢包统计、RSSI 记录 | 丢包率 / 延迟数据表 |
| **D90** 周六 | ★★ 异构计算：任务卸载设计 | 同一推理任务三方案：C3 本地 / 卸载到 S3 / 上传云端；测端到端延迟、电量、准确率、断网可用性；写入**端云数据分级**判据、架构图、数据契约、降级路径 | `notes/offload-design.md`：四维对比表 + "何时卸载"判定树；接口/超时/降级全定义 |
| **D91** 周日 | 复盘 + 博客 #13 | 《SMP 与异构卸载：双核不是把活分两半》 | **阶段 Gate P6**：两板稳定互通 ≥1h；能脱稿讲清自旋锁/绑核/卸载决策 |

---

### W14 · D92–D98 ｜ P7 旗舰项目集成

**旗舰项目**：**「低功耗多节点环境感知与声控边缘系统」**
C3 传感节点（SHT30 + MPU6050 + 麦克风 + TinyML + deep sleep + ESP-NOW）→ C3/S3 网关（聚合 + 规则 + MQTT + OTA + Web 配置）→ PC 端 mosquitto + InfluxDB + Grafana。

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D92** 周一 | ★★ 系统设计定稿 | 需求/架构/分区表/任务划分/状态机/数据契约/失败矩阵/**PID 闭环回路与控制周期预算** → `90_flagship/DESIGN.md` | 设计文档自评 checklist ≥20 项全过 |
| **D93** 周二 | ★★ 多任务架构实现 | 采集/推理/通信/规则/UI 五任务 + 消息流 + 优先级分配 | 各任务 CPU 与栈用量达标（总 CPU <70%） |
| **D94** 周三 | ★★ 可靠性加固 ⚠️ | WDT 全覆盖、异常重启计数、降级模式（断网→本地决策，模型错→规则兜底）、OTA 回滚 | **10 种故障注入全部自恢复** |
| **D95** 周四 | ★ 功耗与性能打磨 | 唤醒周期、批量上报、推理频率自适应、**功耗预算模型与续航反推**、DVFS | 续航提升量化（如 3.5 天 → 9 天）；估算 vs 实测偏差 <20% |
| **D96** 周五 | ★ 自动化测试 | pytest-embedded：烧录→跑用例→断言，覆盖启动/联网/推理/OTA | **一条命令跑完全套**，报告可读 |
| **D97** 周六 | ★ CI 与交付 | GitHub Actions：编译 + 体积守门 + 单测 + Release（tag + bin） | 干净 clone 一条命令构建成功；Release 可下载 |
| **D98** 周日 | 复盘 + 博客 #14 | 《一个能交付的固件长什么样：分区、OTA、CI 与测试》 | **阶段 Gate**：外人按 README 能跑起来 |

**10 种故障注入清单（D94 用）**：① 拔电源 ② 断 WiFi ③ 断 MQTT ④ 传感器拔线 ⑤ 模型文件损坏 ⑥ Flash 写满 ⑦ 堆耗尽 ⑧ 任务卡死 ⑨ OTA 中途断电 ⑩ 电压跌落 brownout

---

### W15 · D99–D105 ｜ P7 工程化与求职素材

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D99** 周一 | 代码质量 | cppcheck / clang-tidy 配到全仓；MISRA 关键规则自查 | 高危告警清零，附检查报告 |
| **D100** 周二 | 文档体系 | README（架构/构建/使用/排障）+ Doxygen API + 演示脚本 | 别人照文档 30min 内跑通 |
| **D101** 周三 | ★ 简历项目提炼 | STAR 写 3 条：①底层架构分析 ②TinyML 落地 ③可靠性设计 | 每条都有**可量化指标**（如"中断延迟 12µs → 3.2µs"） |
| **D102** 周四 | ★ 高频八股整理 | 100 题：C 语言 / RTOS / 驱动 / 总线协议 / 网络 / 底层 → `notes/interview.md` | 自测答出 ≥80% |
| **D103** 周五 | ★ 手写代码训练 | 环形缓冲、位图、状态机、CRC、链表反转、`memcpy` 优化、字符串库 | 无编译警告 + 边界用例全过 |
| **D104** 周六 | ★★ 模拟面试与演示 | 录 10min 项目演示；自问自答 20 问（为什么用 C3 不用 S3？为什么不用 RTOS 的 xxx？） | 3 分钟内讲清架构与关键取舍 |
| **D105** 周日 | 复盘 + 博客 #15 | 《从学习到可入职：我给自己的验收标准》 | **求职 Gate**：博客公开发布 ≥8 篇 |

---

### W16 · D106–D112 ｜ 收尾与定向补强

| 天 | 主题 | 产出 | 🎯 验收 |
|---|---|---|---|
| **D106** 周一 | ★ 缺口诊断 | 按 `notes` 附册 F 节"精通 10 条"逐条打分（1–5），列缺口清单 | 缺口清单按优先级排序，进 D107–D109 |
| **D107** 周二 | 定向补强 A | **BLE 协议栈深挖**：抓包看 ATT/GATT 报文、配对绑定与加密（双核未达标则此日优先回补） | 能用抓包解释一次完整配对过程 |
| **D108** 周三 | 定向补强 B | **RTOS 内核源码精读**：`vTaskSwitchContext`、`queue.c` 阻塞链表、SMP 仿射掩码选核 | 能口述一次上下文切换的完整代码路径 |
| **D109** 周四 | 定向补强 C | **Linux 侧能力**：串口协议对接脚本、socket、systemd 服务 | 一条 systemd 服务稳定采数 ≥10min |
| **D110** 周五 | ★ 对外产出 | 给开源 IDF 组件提 1 个 PR，或自家组件发 v1.1（含 CHANGELOG） | PR 已提交 / tag 已打 |
| **D111** 周六 | ★ 总复盘 | 112 天数据回顾（代码行数/测量数据/博客数/踩坑数）+ 下一个 90 天 | 数据表 + 新计划草案 |
| **D112** 周日 | 缓冲与收官 | 清理欠账、整仓 tag `v1.0` | `git tag` 齐全，README 终版 |

---

## 附：开工前 30 秒检查

```powershell
$HOME\esp\esp-idf\export.ps1
idf.py --version                    # 期望 v5.x
where.exe idf.py                    # 期望只有一处
New-Item -ItemType Directory E:\c3-lab -Force; cd E:\c3-lab; git init
```
