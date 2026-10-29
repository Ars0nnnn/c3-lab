// include/c3lab/gpio.hpp
#pragma once
#include "c3lab/gpio_regs.hpp"
#include <cstdint>
#include <utility> // std::to_underlying (C++23，你的 gnu++26 有)

namespace c3lab {

enum class io : std::uint8_t { // 类型安全：不会把 4 当 int 乱传
  btn1 = 4,
  btn2 = 5,
  btn3 = 6,
  d1 = 10,
  d2 = 11,
  d3 = 12,
};

constexpr std::uint8_t num(io p) noexcept { return std::to_underlying(p); }
constexpr std::uint32_t mask(io p) noexcept { return mmio::bit(num(p)); }
static_assert(num(io::btn1) == 4 && mask(io::d1) == 1u << 10);

namespace gpio {

inline void func_gpio(io p) noexcept {
  mmio::reg32<0>::set_bits(0); // 占位，见下面真正的实现
}

inline void input_pullup(io p) noexcept {
  static_assert(num(io::btn1) < 32); // 本工程只用 bank0
  const auto mux = gpio_regs::iomux(num(p));
  // MCU_SEL=1 + FUN_IE + FUN_PU，一次 RMW 写完（初始化阶段无并发，安全）
  *reinterpret_cast<volatile std::uint32_t *>(mux) =
      gpio_regs::mcu_sel_gpio | gpio_regs::fun_ie | gpio_regs::fun_pu;
  mmio::reg32<gpio_regs::enable_w1tc()>::write(mask(p)); // 关输出
}

inline void output(io p) noexcept {
  const auto mux = gpio_regs::iomux(num(p));
  *reinterpret_cast<volatile std::uint32_t *>(mux) =
      gpio_regs::mcu_sel_gpio;                           // 无上拉、关输入缓冲
  low(p);                                                // 先定电平
  mmio::reg32<gpio_regs::enable_w1ts()>::write(mask(p)); // 再开输出
}

inline void high(io p) noexcept {
  if constexpr (num(p) < 32)
    mmio::reg32<gpio_regs::out_w1ts()>::write(mask(p));
}
inline void low(io p) noexcept {
  if constexpr (num(p) < 32)
    mmio::reg32<gpio_regs::out_w1tc()>::write(mask(p));
}

[[nodiscard]] inline bool level(io p) noexcept {
  return (mmio::reg32<gpio_regs::in_reg()>::read() & mask(p)) != 0;
}

} // namespace gpio
} // namespace c3lab
