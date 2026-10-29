// common_components/c3lab_gpio/include/c3lab/gpio_regs.hpp
//
// ESP32-S3 的 GPIO / IO_MUX 寄存器地址表 + 位域常量。
// 数值来源：components/soc/esp32s3/register/soc/{gpio_reg,io_mux_reg}.h（IDF v6.1）
//
// 为什么不用 IDF 的 REG_WRITE(GPIO_OUT_W1TS_REG)？
//   1) 宏在全局命名空间，拼错要到链接期才报错
//   2) 没有类型，无法参与模板
//   3) 这份表可以 static_assert 自检，地址写错编译当場失败
#pragma once

#include <cstdint>

namespace c3lab::gpio_regs
{

// ---- 外设基址 ------------------------------------------------------------
inline constexpr std::uintptr_t gpio_base  = 0x6000'4000;  // GPIO  外设
inline constexpr std::uintptr_t iomux_base = 0x6000'9000;  // IO_MUX 外设

// ---- GPIO 外设寄存器偏移 -------------------------------------------------
// bank0 = 引脚 0..31，bank1 = 引脚 32..48（S3 最高到 48）
constexpr std::uintptr_t out_reg()          noexcept { return gpio_base + 0x04; }
constexpr std::uintptr_t out_w1ts()         noexcept { return gpio_base + 0x08; }
constexpr std::uintptr_t out_w1tc()         noexcept { return gpio_base + 0x0C; }
constexpr std::uintptr_t out1_w1ts()        noexcept { return gpio_base + 0x14; }
constexpr std::uintptr_t out1_w1tc()        noexcept { re