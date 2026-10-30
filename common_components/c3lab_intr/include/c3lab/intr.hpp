#pragma once

#include <cstdint>

#include "esp_err.h"
#include "esp_intr_alloc.h"

#include "c3lab/interrupts_regs.hpp"
#include "c3lab/mmio.hpp"

namespace c3lab::intr {

using isr_t =
    void (*)(void *); // 不用 std::function（check-cpp 禁 + 有堆分配风险）

enum class level : std::uint8_t { l1 = 1, l2 = 2, l3 = 3 };

struct handle {
  intr_handle_t raw = nullptr; // IDF 的不透明句柄，转发用
  std::uint32_t source = 0;    // 缓存下来：审计和调试要用
};

[[nodiscard]] inline std::uint32_t cpu_int_of(std::uint32_t source) noexcept {
  return static_cast<std::uint32_t>(
             c3lab::mmio::read_at(c3lab::intr_regs::map_reg(source))) &
         c3lab::intr_regs::intno_mask;
}

inline constexpr std::uint32_t map_disabled_intno = 6; // IDF 用它表示"已关闭"
[[nodiscard]] inline bool is_routed(std::uint32_t source) noexcept {
  return cpu_int_of(source) != map_disabled_intno;
}

// 分配cpu中断号，6为已关闭
[[nodiscard]] esp_err_t alloc(std::uint32_t source, level lv, isr_t fn,
                              void *arg, handle &out) noexcept;
// 释放cpu中断号
[[nodiscard]] esp_err_t free(handle &h) noexcept;
// 启用cpu中断
[[nodiscard]] inline esp_err_t enable(handle h) noexcept {
  if (h.raw == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  return esp_intr_enable(h.raw); // IDF 内部：重写矩阵路由
}

[[nodiscard]] inline esp_err_t disable(handle h) noexcept {
  if (h.raw == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  return esp_intr_disable(h.raw); // IDF 内部：路由到 6 = 关
}

} // namespace c3lab::intr