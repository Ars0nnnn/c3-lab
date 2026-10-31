// include/c3lab/timer.hpp
#pragma once

#include <cstdint>

#include "esp_err.h"
#include "soc/clk_tree_defs.h" // soc_module_clk_t / SOC_MOD_CLK_*

namespace c3lab::timer::ledc {

// 时钟源：枚举值 = 硬件 LEDC_CONF.apb_clk_sel 的编码（1/2/3）。
// 注意不是 ledc_clk_cfg_t 的 {4,9,11}——那套是给 IDF 驱动用的别名值。

enum class LedcClkReg : uint32_t {
    Apb = 1,    // 80 MHz  APB 总线
    RcFast = 2, // ~17.5 MHz 内部 RC（需标定）
    Xtal = 3,   // 40 MHz 外部晶振
};

// 全局时钟源（全组 4 个 timer 共享，只在 Init 时定一次）
struct ClkCfg {
    LedcClkReg reg;          // → LEDC.conf.apb_clk_sel
    soc_module_clk_t module; // → 向时钟树要实际频率
};

constexpr ClkCfg kClkApb{LedcClkReg::Apb, SOC_MOD_CLK_APB};
constexpr ClkCfg kClkXtal{LedcClkReg::Xtal, SOC_MOD_CLK_XTAL};
constexpr ClkCfg kClkRcFast{LedcClkReg::RcFast, SOC_MOD_CLK_RC_FAST};

struct TimerCfg {
    uint32_t timer;    // 0..SOC_LEDC_TIMER_NUM-1
    uint32_t freq_hz;  // 期望 PWM 频率
    uint32_t res_bits; // 1..SOC_LEDC_TIMER_BIT_WIDTH（S3 是 14）
};

// —— 纯计算：可单测、不碰硬件 ——
// 返回值 0 表示"该组合不可实现"（对应 IDF 的 LEDC_CLK_NOT_FOUND 语义）
[[nodiscard]] constexpr uint32_t CalcDivReg(uint32_t src_hz, uint32_t freq_hz,
                                            uint32_t res_bits) {
    const uint64_t precision = 1ull << res_bits;
    if (freq_hz == 0 || res_bits == 0)
        return 0;
    return static_cast<uint32_t>(
        ((static_cast<uint64_t>(src_hz) << 8) + freq_hz * precision / 2) /
        (freq_hz * precision));
}

[[nodiscard]] constexpr bool DivIsValid(uint32_t div_reg) {
    return div_reg > 0xFFu &&
           div_reg <= 0x3FFFFu; // 256 .. 1023.996，对齐 IDF 判据
}

// —— 两阶段初始化：全局一次 → 每 timer 一次 ——
[[nodiscard]] esp_err_t Init(const ClkCfg &clk); // 门控 + 复位 + 接源 + 取频率
[[nodiscard]] esp_err_t
TimerInit(const TimerCfg &cfg); // 算 div + 写寄存器 + 启动

// —— 运行时 ——
[[nodiscard]] esp_err_t TimerSetFreq(uint32_t timer, uint32_t freq_hz);
[[nodiscard]] esp_err_t TimerGetFreq(uint32_t timer, uint32_t &out_hz);
[[nodiscard]] esp_err_t TimerGetDiv(uint32_t timer, uint32_t &out_div_reg);
[[nodiscard]] esp_err_t TimerMaxDuty(uint32_t timer, uint32_t &out_max);
[[nodiscard]] esp_err_t TimerStop(uint32_t timer);

} // namespace c3lab::timer::ledc