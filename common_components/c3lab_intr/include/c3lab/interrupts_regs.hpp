#pragma once

#include <cstdint>

namespace c3lab::intr_regs {
inline constexpr std::uintptr_t matrix_base =
    0x600'C2000u; // 写错成 0x6000C200 就废
constexpr std::uintptr_t map_reg(std::uint32_t source) noexcept {
  return matrix_base + 4u * source;
}
inline constexpr std::uint32_t intno_mask = 0x1Fu;

static_assert(map_reg(16) ==
              0x600C2040u); // GPIO_INTERRUPT_PRO_MAP_REG  = base + 0x040
static_assert(map_reg(50) ==
              0x600C20C8u); // TG0_T0_LEVEL                = base + 0x0C8
static_assert(map_reg(79) ==
              0x600C213Cu); // CPU_INTR_FROM_CPU_0         = base + 0x13C
} // namespace c3lab::intr_regs