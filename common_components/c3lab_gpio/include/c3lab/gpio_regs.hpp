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
constexpr std::uintptr_t out1_reg()         noexcept { return gpio_base + 0x10; }
constexpr std::uintptr_t out1_w1ts()        noexcept { return gpio_base + 0x14; }
constexpr std::uintptr_t out1_w1tc()        noexcept { return gpio_base + 0x18; }
constexpr std::uintptr_t enable_reg()       noexcept { return gpio_base + 0x20; }
constexpr std::uintptr_t enable_w1ts()      noexcept { return gpio_base + 0x24; }
constexpr std::uintptr_t enable_w1tc()      noexcept { return gpio_base + 0x28; }
constexpr std::uintptr_t enable1_reg()      noexcept { return gpio_base + 0x2C; }
constexpr std::uintptr_t enable1_w1ts()     noexcept { return gpio_base + 0x30; }
constexpr std::uintptr_t enable1_w1tc()     noexcept { return gpio_base + 0x34; }
constexpr std::uintptr_t in_reg()           noexcept { return gpio_base + 0x3C; }
constexpr std::uintptr_t in1_reg()          noexcept { return gpio_base + 0x40; }
constexpr std::uintptr_t status_reg()       noexcept { return gpio_base + 0x44; }
constexpr std::uintptr_t status_w1tc()      noexcept { return gpio_base + 0x4C; }
constexpr std::uintptr_t status1_reg()      noexcept { return gpio_base + 0x50; }
constexpr std::uintptr_t status1_w1tc()     noexcept { return gpio_base + 0x58; }

// 每引脚的配置寄存器：GPIO.pin[n]，n 从 0 开始的 4 字节步进
constexpr std::uintptr_t pin_conf(std::uint8_t n) noexcept
{
    return gpio_base + 0x74u + 4u * n;
}

// ---- IO_MUX 寄存器 -------------------------------------------------------
// GPIO0 在 +0x04，之后每脚 +4 字节连续排布
constexpr std::uintptr_t iomux(std::uint8_t n) noexcept
{
    return iomux_base + 0x04u + 4u * n;
}

// ---- IO_MUX 位域（S3）---------------------------------------------------
inline constexpr std::uint32_t fun_pd       = 1u << 7;   // 下拉
inline constexpr std::uint32_t fun_pu       = 1u << 8;   // 上拉
inline constexpr std::uint32_t fun_ie       = 1u << 9;   // 输入缓冲使能
inline constexpr std::uint32_t fun_drv_mask = 3u << 10;  // 驱动强度
inline constexpr std::uint32_t mcu_sel_mask = 7u << 12;  // 功能选择
inline constexpr std::uint32_t mcu_sel_gpio = 1u << 12;  // PIN_FUNC_GPIO = 1

// ---- 中断触发类型（与 GPIO_INTR_* 枚举同值）-----------------------------
namespace intr
{
inline constexpr std::uint8_t disable  = 0;
inline constexpr std::uint8_t posedge  = 1;
inline constexpr std::uint8_t negedge  = 2;
inline constexpr std::uint8_t anyedge  = 3;
inline constexpr std::uint8_t low      = 4;
inline constexpr std::uint8_t high     = 5;
} // namespace intr

// ---- 编译期自检：地址算错，here we fail --------------------------------
// 这四行就是"和芯片手册对账"的唯一凭据，改基址/步进会立刻炸
static_assert(iomux(0)  == 0x6000'9004);
static_assert(iomux(4)  == 0x6000'9014);
static_assert(iomux(10) == 0x6000'902C);
static_assert(iomux(48) == 0x6000'90C4);
static_assert(pin_conf(0) == 0x6000'4074);

} // namespace c3lab::gpio_regs
