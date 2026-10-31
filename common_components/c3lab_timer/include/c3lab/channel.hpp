// include/c3lab/channel.hpp
#pragma once

#include <cstdint>

#include "esp_err.h"

namespace c3lab::timer::ledc {

struct ChannelCfg {
    uint32_t channel; // 0..SOC_LEDC_CHANNEL_NUM-1
    uint32_t timer;   // 绑哪个 timer（决定 freq/res）
    int gpio;         // 输出引脚；-1 = 不接 GPIO（只做内部信号）
    bool invert;      // true = 反相输出（低有效负载）
    bool idle_high;   // sig_out_en=0 时引脚停在"高"还是"低"（反相前）
    uint32_t hpoint;  // 相位，通常 0
    uint32_t duty;    // 初始 duty（tick 数，0..2^res）
};

[[nodiscard]] esp_err_t ChannelInit(const ChannelCfg &cfg);

// 运行时改占空比（只写 duty + 通道 PARA_UP，不碰 conf1）
[[nodiscard]] esp_err_t SetDuty(uint32_t channel, uint32_t duty);
[[nodiscard]] esp_err_t SetDutyPermille(uint32_t channel,
                                        uint32_t permille); // 0..1000
[[nodiscard]] esp_err_t GetDuty(uint32_t channel, uint32_t &out);

// 安全停机：idle_lv + sig_out_en=0（不是 duty=0）
[[nodiscard]] esp_err_t ChannelStop(uint32_t channel, bool idle_high);

// 换绑 timer 时用：把 duty 按新旧 res 重新标定
[[nodiscard]] esp_err_t RebindTimer(uint32_t channel, uint32_t new_timer);

} // namespace c3lab::timer::ledc